# define CATCH_CONFIG_RUNNER
# include <catch2/catch.hpp>
# include <Siv3D.hpp>

SIV3D_SET(EngineOption::Renderer::Headless)

void Main()
{
	Catch::Session session;

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
