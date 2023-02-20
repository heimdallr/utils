#include <iostream>
#include <vector>
#include <unordered_map>

#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QString>

namespace {

using File = std::pair<QString, QString>;
using Files = std::vector<File>;

constexpr auto REMOVED = "/removed/";

Files GetFileList(const QString & path)
{
	Files result;
	QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		it.next();
		const auto fileInfo = it.fileInfo();
		result.emplace_back(fileInfo.path(), fileInfo.fileName());
	}
	return result;
}

void Process(const QString & path)
{
	if (QDir(path + REMOVED).exists())
		throw std::ios_base::failure(QString("%1%2 must not exists").arg(path, REMOVED).toStdString());

	auto files = GetFileList(path);
	std::cout << std::size(files) << " files found" << std::endl;

	std::unordered_map<std::string, Files> copies;
	QCryptographicHash hash(QCryptographicHash::Md5);

	for (size_t i = 0, currentPercents = 0, sz = std::size(files); i < sz; ++i)
	{
		const auto filePath = QDir::fromNativeSeparators(files[i].first + QDir::separator() + files[i].second);
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
			throw std::ios_base::failure(QString("Cannot open %1").arg(filePath).toStdString());

		hash.reset();
		if (!hash.addData(&file))
			throw std::ios_base::failure(QString("Cannot read %1").arg(filePath).toStdString());

		copies[hash.result().toHex().toStdString()].push_back(std::move(files[i]));

		if (const auto percents = (i + 1) * 100 / sz; currentPercents != percents)
		{
			std::cout << "Find copies: " << i + 1 << " (" << sz << ")" << std::endl;
			currentPercents = percents;
		}
	}

	for (auto it = copies.begin(); it != copies.end(); )
		if (it->second.size() > 1)
			++it;
		else
			it = copies.erase(it);

	std::cout << copies.size() << " copies found" << std::endl;

	auto it = copies.begin();
	for (size_t i = 0, currentPercents = 0, sz = copies.size(); i < sz; ++i, ++it)
	{
		assert(it != copies.end());

		const auto maxPathIt = std::ranges::max_element(it->second, [] (const File & lhs, const File & rhs)
		{
			const auto lhsL = lhs.first.length(), rhsL = rhs.first.length();
			return lhs.first.length() < rhs.first.length();
		});

		const auto lastPathIt = std::next(it->second.begin(), it->second.size() - 1);
		if (maxPathIt != lastPathIt)
			std::swap(*maxPathIt, *lastPathIt);

		it->second.pop_back();

		for (const auto & file : it->second)
		{
			const QFileInfo dstInfo(path + REMOVED + file.first.mid(path.length()) + QDir::separator() + file.second);
			const auto dstPath = dstInfo.filePath();
			if (!QDir(dstInfo.path()).exists())
				(void)QDir().mkdir(dstInfo.path());

			QFile::rename(QDir::fromNativeSeparators(file.first + QDir::separator() + file.second), dstInfo.filePath());
		}

		if (const auto percents = (i + 1) * 100 / sz; currentPercents != percents)
		{
			std::cout << "Remove copies: " << i + 1 << " (" << sz << ")" << std::endl;
			currentPercents = percents;
		}
	}
}

int MainImpl(int argc, wchar_t * argv[])
{
	if (argc < 2)
		throw std::invalid_argument("need path as command line parameter");

	Process(QDir::fromNativeSeparators(QString::fromStdWString(argv[1])));
	return 0;
}

}

int wmain(int argc, wchar_t * argv[])
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
