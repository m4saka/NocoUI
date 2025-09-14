#pragma once
#include "YN.hpp"

namespace noco
{
	void Init(AddNocoUILicenseYN addNocoUILicense = AddNocoUILicenseYN::Yes, AddMagicEnumLicenseYN addMagicEnumLicense = AddMagicEnumLicenseYN::Yes);

	bool IsInitialized();

	void AddNocoUILicense();

	void AddMagicEnumLicense();
}
