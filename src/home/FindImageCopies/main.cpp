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
using Copies = std::unordered_map<std::string, Files>;

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

Copies FindCopies(Files && files)
{
	Copies copies;
	QCryptographicHash hash(QCryptographicHash::Md5);

	for (size_t i = 0, sz = std::size(files); i < sz; ++i)
	{
		const auto filePath = QDir::fromNativeSeparators(files[i].first + QDir::separator() + files[i].second);
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
			throw std::ios_base::failure(QString("Cannot open %1").arg(filePath).toStdString());

		hash.reset();
		if (!hash.addData(&file))
			throw std::ios_base::failure(QString("Cannot read %1").arg(filePath).toStdString());

		copies[hash.result().toHex().toStdString()].push_back(std::move(files[i]));

		if ((i + 1) % 100 == 0)
			std::cout << "Copies searching: " << i + 1 << " (" << sz << ") - " << (i + 1) * 100 / sz << "%" << std::endl;
	}

	for (auto it = copies.begin(); it != copies.end(); )
		if (it->second.size() > 1)
			++it;
		else
			it = copies.erase(it);

	return copies;
}

void MoveCopies(const QString & path, Copies && copies)
{
	for (auto it = copies.begin(), end = copies.end(); it != end; ++it)
	{
		assert(it != copies.end());

		const auto maxPathIt = std::ranges::max_element(it->second, [] (const File & lhs, const File & rhs)
		{
			return lhs.first.length() < rhs.first.length();
		});

		const auto lastPathIt = std::next(it->second.begin(), it->second.size() - 1);
		if (maxPathIt != lastPathIt)
			std::swap(*maxPathIt, *lastPathIt);

		std::cout << it->first << ": " << QString("%1/%2").arg(it->second.back().first, it->second.back().second).toStdString() << std::endl;
		it->second.pop_back();

		for (const auto & file : it->second)
		{
			const QFileInfo dstInfo(path + REMOVED + file.first.mid(path.length() + 1) + QDir::separator() + file.second);
			const auto dstDir = dstInfo.path();
			if (!QDir(dstDir).exists() && !QDir().mkpath(dstDir))
				throw std::ios_base::failure(QString("Cannot create %1 for %2").arg(dstDir, file.second).toStdString());

			if (QFile::rename(QDir::fromNativeSeparators(file.first + QDir::separator() + file.second), dstInfo.filePath()))
				std::cout << QString("%1/%2").arg(file.first, file.second).toStdString() << std::endl;
			else
				std::cerr << "Cannot move " << file.second.toStdString() << " from " << file.second.toStdString() << " to " << dstInfo.path().toStdString() << std::endl;
		}
	}
}

void Process(QString path)
{
	if (path.back() == '/')
		path.resize(path.length() - 1);

	if (QDir(path + REMOVED).exists())
		throw std::ios_base::failure(QString("%1%2 must not exists").arg(path, REMOVED).toStdString());

	auto files = GetFileList(path);
	std::cout << std::size(files) << " files found" << std::endl;

	auto copies = FindCopies(std::move(files));
	std::cout << copies.size() << " copies found" << std::endl;

	MoveCopies(path, std::move(copies));
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
