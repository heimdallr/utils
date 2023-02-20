#include <iostream>

#include <QString>

namespace {

void Process(const QString & /*path*/)
{
}

int MainImpl(int argc, wchar_t * argv[])
{
	if (argc < 2)
		throw std::invalid_argument("need path as command line parameter");

	Process(QString::fromStdWString(argv[1]));
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
