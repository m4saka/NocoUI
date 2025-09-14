#include "NocoUI/Init.hpp"

namespace
{
	bool s_initialized = false;
}

namespace noco
{
	void Init(AddNocoUILicenseYN addNocoUILicense, AddMagicEnumLicenseYN addMagicEnumLicense)
	{
		if (s_initialized)
		{
			throw Error{ U"noco::Init has already been called." };
		}

		if (addNocoUILicense)
		{
			AddNocoUILicense();
		}
		if (addMagicEnumLicense)
		{
			AddMagicEnumLicense();
		}

		s_initialized = true;
	}

	bool IsInitialized()
	{
		return s_initialized;
	}

	void AddNocoUILicense()
	{
		LicenseManager::AddLicense(
			LicenseInfo
			{
				UR"-(NocoUI)-",
				UR"-(Copyright (c) 2025 masaka)-",
				UR"-(Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)-",
			});
	}

	void AddMagicEnumLicense()
	{
		LicenseManager::AddLicense(
			LicenseInfo
			{
				UR"-(magic_enum)-",
				UR"-(Copyright (c) 2019 - 2024 Daniil Goncharov)-",
				UR"-(Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)-",
			});
	}
}
