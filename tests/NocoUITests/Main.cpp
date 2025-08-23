# define CATCH_CONFIG_RUNNER
# include <catch2/catch.hpp>
# include <Siv3D.hpp>

// Linux環境でのみヘッドレスモードを使用
#ifdef __linux__
SIV3D_SET(EngineOption::Renderer::Headless)
#endif

void Main()
{
	Catch::Session session;

	// コマンドライン引数を取得
	const Array<String> args = System::GetCommandLineArgs();
	
	// argc, argvに変換
	std::vector<char*> argv;
	std::vector<std::string> argStrings;
	
	for (const auto& arg : args)
	{
		argStrings.push_back(arg.toUTF8());
	}
	
	for (auto& argString : argStrings)
	{
		argv.push_back(const_cast<char*>(argString.c_str()));
	}
	
	int argc = static_cast<int>(argv.size());

	// Catchにコマンドライン引数を渡す
	session.applyCommandLine(argc, argv.data());

	int numFailed = session.run();
	if (numFailed == 0)
	{
		Console << U"All tests passed!";
	}
	else
	{
		Console << U"Tests failed: " << numFailed;
	}
}
