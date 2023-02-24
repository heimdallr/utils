#include <iostream>
#include <vector>
#include <unordered_map>

#include <QApplication>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QImageReader>
#include <QString>

namespace {

using File = std::pair<QString, QString>;
using Files = std::vector<File>;

constexpr auto REMOVED = "/removed/";

class QtLogHandler;
QtLogHandler * g_qtLogHandler = nullptr;

class QtLogHandler
{
public:
	mutable QString lastMessage;

public:
	QtLogHandler()
	{
		m_previousHandler = qInstallMessageHandler(&QtLogHandler::HandlerStatic);
		g_qtLogHandler = this;
	}

	~QtLogHandler()
	{
		g_qtLogHandler = nullptr;
		qInstallMessageHandler(m_previousHandler);
	}

private:
	void Handler(QtMsgType type, const QMessageLogContext & /*ctx*/, const QString & message) const
	{
		switch (type)
		{
			case QtDebugMsg:
			case QtInfoMsg:
				break;
			case QtWarningMsg:
			case QtCriticalMsg:
			case QtFatalMsg:
				lastMessage = message;
				break;
			default:
				assert(false);
		}
	}

	static void HandlerStatic(QtMsgType type, const QMessageLogContext & ctx, const QString & message)
	{
		assert(g_qtLogHandler);
		g_qtLogHandler->Handler(type, ctx, message);
	}

private:
	QtMessageHandler m_previousHandler { qInstallMessageHandler(&QtLogHandler::HandlerStatic) };
};

QString GetFilePath(const QString & path, const QString & name)
{
	return QDir::fromNativeSeparators(QString("%1/%2").arg(path, name));
}

QString GetFilePath(const File & file)
{
	return GetFilePath(file.first, file.second);
}

Files GetFileList(const QString & path, const QStringList & filters)
{
	Files result;
	QDirIterator it(path, filters, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		it.next();
		const auto fileInfo = it.fileInfo();
		result.emplace_back(fileInfo.path(), fileInfo.fileName());
	}
	return result;
}

bool ImageNeedRemoved(const File & file, QtLogHandler & logHandler)
{
	const auto filePath = GetFilePath(file);
	std::cout << filePath.toStdString() << std::endl;
	QImage image;
	try
	{
		QImageReader reader(filePath);
		if (!reader.canRead())
		{
			std::cout << "Cannot read " << filePath.toStdString() << std::endl;
			return true;
		}

		if (!reader.read(&image))
		{
			std::cout << "Cannot load " << filePath.toStdString() << std::endl;
			return true;
		}

		if (!logHandler.lastMessage.isEmpty())
		{
			std::cout << "Load failed" << filePath.toStdString() << ": " << logHandler.lastMessage.toStdString() << std::endl;
			logHandler.lastMessage.clear();
			return true;
		}
	}
	catch (const std::exception & ex)
	{
		std::cout << "Cannot load " << filePath.toStdString() << ": " << ex.what() << std::endl;
		return true;
	}
	catch (...)
	{
		std::cout << "Cannot load " << filePath.toStdString() << ": unknown error" << std::endl;
		return true;
	}

	if (image.isNull())
	{
		std::cout << "Pixmap is null" << filePath.toStdString() << std::endl;
		return true;
	}

	if (image.width() == 0 || image.height() == 0)
	{
		std::cout << "Zero pixmap size" << filePath.toStdString() << std::endl;
		return true;
	}

	return false;
}

void MoveInvalidImage(const QString & path, const File & file)
{
	const auto & [filePath, fileName] = file;
	const QFileInfo dstInfo(path + REMOVED + filePath.mid(path.length() + 1) + QDir::separator() + fileName);
	const auto dstDir = dstInfo.path();
	if (!QDir(dstDir).exists() && !QDir().mkpath(dstDir))
		throw std::ios_base::failure(QString("Cannot create %1 for %2").arg(dstDir, fileName).toStdString());

	const auto src = GetFilePath(filePath, fileName);
	if (QFile::rename(src, dstInfo.filePath()))
		std::cout << src.toStdString() << " moved to " << dstInfo.path().toStdString() << std::endl;
	else
		std::cerr << "Cannot move " << fileName.toStdString() << " from " << fileName.toStdString() << " to " << dstInfo.path().toStdString() << std::endl;
}

size_t FindInvalidImages(const QString & path, const Files & files, QtLogHandler & logHandler)
{
	size_t n = 0;
	std::ranges::for_each(files, [&, i = size_t {}, sz = files.size()](const File & file) mutable
	{
		if (++i % 10 == 0)
			std::cout << "Invalid images searching: " << i << " (" << sz << ") - " << i * 100 / sz << "%, " << "invalid images found: " << n << std::endl;

		if (ImageNeedRemoved(file, logHandler))
		{
			MoveInvalidImage(path, file);
			++n;
		}
	});

	return n;
}

void Process(QString path, QStringList filters, QtLogHandler & qtLogHandler)
{
	if (path.back() == '/')
		path.resize(path.length() - 1);

	if (QDir(path + REMOVED).exists())
		throw std::ios_base::failure(QString("%1%2 must not exists").arg(path, REMOVED).toStdString());

	auto files = GetFileList(path, filters);
	std::cout << std::size(files) << " files found" << std::endl;

	const auto invalidImageCount = FindInvalidImages(path, std::move(files), qtLogHandler);
	std::cout << invalidImageCount << " invalid images found" << std::endl;
}

int MainImpl(int argc, char * argv[])
{
	if (argc < 2)
		throw std::invalid_argument("Usage:\nFindInvalidImages path [mask](default *.jpg)");

	QApplication app(argc, argv);
	QtLogHandler qtLogHandler;

	QStringList filters;
	for (int i = 2; i < argc; ++i)
		filters << argv[i];
	if (filters.isEmpty())
		filters << "*.jpg";

	Process(QDir::fromNativeSeparators(argv[1]), std::move(filters), qtLogHandler);
	return 0;
}

}

int main(int argc, char * argv[])
{
	try
	{
		return MainImpl(argc, argv);
	}
	catch(const std::exception & ex)
	{
		std::cerr << ex.what();
	}
	catch(...)
	{
		std::cerr << "Unknown error";
	}
	return 1;
}
