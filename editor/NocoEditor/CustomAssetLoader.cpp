#include "CustomAssetLoader.hpp"
#include <NocoUI/Serialization.hpp>
#include "ComponentSchemaLoader.hpp"

namespace noco::editor
{
	namespace
	{
		enum class FontSourceType : uint8
		{
			File,
			Typeface,
		};

		// Siv3DのTypefaceは列挙子にエイリアスを含みmagic_enumで使用できないため別途定義
		enum class TypefaceType : uint8
		{
			CJK_Regular_JP,
			CJK_Regular_KR,
			CJK_Regular_SC,
			CJK_Regular_TC,
			CJK_Regular_HK,
			MonochromeEmoji,
			ColorEmoji,
			Mplus_Thin,
			Mplus_Light,
			Mplus_Regular,
			Mplus_Medium,
			Mplus_Bold,
			Mplus_Heavy,
			Mplus_Black,
			Icon_Awesome_Solid,
			Icon_Awesome_Brand,
			Icon_MaterialDesign,
			Thin,
			Light,
			Regular,
			Medium,
			Bold,
			Heavy,
			Black,
		};

		[[nodiscard]]
		Typeface ConvertToS3dTypeface(TypefaceType type)
		{
			switch (type)
			{
			case TypefaceType::CJK_Regular_JP:
				return Typeface::CJK_Regular_JP;
			case TypefaceType::CJK_Regular_KR:
				return Typeface::CJK_Regular_KR;
			case TypefaceType::CJK_Regular_SC:
				return Typeface::CJK_Regular_SC;
			case TypefaceType::CJK_Regular_TC:
				return Typeface::CJK_Regular_TC;
			case TypefaceType::CJK_Regular_HK:
				return Typeface::CJK_Regular_HK;
			case TypefaceType::MonochromeEmoji:
				return Typeface::MonochromeEmoji;
			case TypefaceType::ColorEmoji:
				return Typeface::ColorEmoji;
			case TypefaceType::Mplus_Thin:
				return Typeface::Mplus_Thin;
			case TypefaceType::Mplus_Light:
				return Typeface::Mplus_Light;
			case TypefaceType::Mplus_Regular:
				return Typeface::Mplus_Regular;
			case TypefaceType::Mplus_Medium:
				return Typeface::Mplus_Medium;
			case TypefaceType::Mplus_Bold:
				return Typeface::Mplus_Bold;
			case TypefaceType::Mplus_Heavy:
				return Typeface::Mplus_Heavy;
			case TypefaceType::Mplus_Black:
				return Typeface::Mplus_Black;
			case TypefaceType::Icon_Awesome_Solid:
				return Typeface::Icon_Awesome_Solid;
			case TypefaceType::Icon_Awesome_Brand:
				return Typeface::Icon_Awesome_Brand;
			case TypefaceType::Icon_MaterialDesign:
				return Typeface::Icon_MaterialDesign;
			case TypefaceType::Thin:
				return Typeface::Thin;
			case TypefaceType::Light:
				return Typeface::Light;
			case TypefaceType::Regular:
				return Typeface::Regular;
			case TypefaceType::Medium:
				return Typeface::Medium;
			case TypefaceType::Bold:
				return Typeface::Bold;
			case TypefaceType::Heavy:
				return Typeface::Heavy;
			case TypefaceType::Black:
				return Typeface::Black;
			default:
				return Typeface::Mplus_Regular;
			}
		}
	}

	void CustomAssetLoader::reload(const Optional<FilePath>& nocoFilePath)
	{
		// 前回登録したアセットを解除
		for (const auto& name : m_registeredFontAssetNames)
		{
			FontAsset::Unregister(name);
		}
		m_registeredFontAssetNames.clear();
		for (const auto& name : m_registeredPixelShaderAssetNames)
		{
			PixelShaderAsset::Unregister(name);
		}
		m_registeredPixelShaderAssetNames.clear();

		const Array<FilePath> directories = ResolveCustomDirectories(nocoFilePath);

		ComponentSchemaLoader::LoadFromDirectories(directories.map([](const FilePath& directory) { return FilePath{ FileSystem::PathAppend(directory, U"Components") }; }));

		for (const auto& directory : directories)
		{
			loadFontAssetsFromDirectory(FileSystem::PathAppend(directory, U"FontAssets"));
			loadPixelShaderAssetsFromDirectory(FileSystem::PathAppend(directory, U"PixelShaders"));
		}
	}

	Array<FilePath> CustomAssetLoader::ResolveCustomDirectories(const Optional<FilePath>& nocoFilePath)
	{
		Array<FilePath> directories;

		if (nocoFilePath)
		{
			// nocoファイルのディレクトリから上位に向かって.nocouiフォルダを探索(近い方が優先)
			FilePath directory = FileSystem::FullPath(FileSystem::ParentPath(*nocoFilePath));
			while (!directory.isEmpty())
			{
				const FilePath nocouiDirectory = FileSystem::PathAppend(directory, U".nocoui");
				if (FileSystem::IsDirectory(nocouiDirectory))
				{
					directories.push_back(nocouiDirectory);
				}
				const FilePath parentDirectory = FileSystem::ParentPath(directory);
				if (parentDirectory == directory)
				{
					break;
				}
				directory = parentDirectory;
			}
		}

		// 実行ファイル側のCustomフォルダは最も優先度が低い
		directories.push_back(FileSystem::PathAppend(FileSystem::ParentPath(FileSystem::ModulePath()), U"Custom"));

		return directories;
	}

	Optional<FilePath> CustomAssetLoader::ResolveAssetBaseDirectoryPath(const FilePath& nocoFilePath)
	{
		// nocoファイルのディレクトリから上の階層へ順に.nocoui/config.jsonを探索(近い方が優先)
		FilePath directory = FileSystem::FullPath(FileSystem::ParentPath(nocoFilePath));
		while (!directory.isEmpty())
		{
			const FilePath configPath = FileSystem::PathAppend(FileSystem::PathAppend(directory, U".nocoui"), U"config.json");
			if (FileSystem::IsFile(configPath))
			{
				const JSON json = JSON::Load(configPath);
				if (not json)
				{
					Logger << U"[NocoEditor warning] Failed to load config JSON: " << configPath;
				}
				else if (json.contains(U"assetBaseDirectoryPath"))
				{
					const String value = json[U"assetBaseDirectoryPath"].getOr<String>(U"");
					if (!value.isEmpty())
					{
						// 相対パスは.nocouiの親ディレクトリ基準で解決(絶対パスはそのまま使用)
						const bool isAbsolute = value.starts_with(U'/') || (value.size() >= 2 && value[1] == U':');
						const FilePath resolved = FileSystem::FullPath(isAbsolute ? value : FilePath{ FileSystem::PathAppend(directory, value) });
						if (FileSystem::IsDirectory(resolved))
						{
							return resolved;
						}
						Logger << U"[NocoEditor warning] assetBaseDirectoryPath '{}' does not exist. Ignoring: {}"_fmt(value, configPath);
					}
				}
			}
			const FilePath parentDirectory = FileSystem::ParentPath(directory);
			if (parentDirectory == directory)
			{
				break;
			}
			directory = parentDirectory;
		}
		return none;
	}

	void CustomAssetLoader::loadFontAssetsFromDirectory(const FilePath& directory)
	{
		if (!FileSystem::IsDirectory(directory))
		{
			return;
		}
		for (const FilePath& path : FileSystem::DirectoryContents(directory, Recursive::Yes))
		{
			if (FileSystem::Extension(path) == U"json")
			{
				loadFontAssetFromFile(path);
			}
		}
	}

	void CustomAssetLoader::loadFontAssetFromFile(const FilePath& path)
	{
		const JSON json = JSON::Load(path);
		if (not json)
		{
			Logger << U"[NocoEditor warning] Failed to load font asset JSON: " << path;
			return;
		}

		const String fontAssetName = json[U"fontAssetName"].getString();
		if (fontAssetName.isEmpty())
		{
			Logger << U"[NocoEditor warning] fontAssetName is empty in: " << path;
			return;
		}

		if (FontAsset::IsRegistered(fontAssetName))
		{
			// 優先度の高いディレクトリで同名のアセットが登録済みの場合はスキップ
			Logger << U"[NocoEditor warning] Font asset '{}' is already registered. Skipping: {}"_fmt(fontAssetName, path);
			return;
		}

		const int32 fontSize = json[U"fontSize"].get<int32>();

		const FontMethod method = json.contains(U"method")
			? StringToEnumOpt<FontMethod>(json[U"method"].getString()).value_or(FontMethod::Bitmap)
			: FontMethod::Bitmap;

		const FontStyle style = json.contains(U"style")
			? StringToEnumOpt<FontStyle>(json[U"style"].getString()).value_or(FontStyle::Default)
			: FontStyle::Default;

		const JSON sourceJson = json[U"source"];
		const auto sourceType = StringToEnumOpt<FontSourceType>(sourceJson[U"type"].getString());
		if (not sourceType)
		{
			Logger << U"[NocoEditor warning] Invalid source type in: " << path;
			return;
		}

		if (*sourceType == FontSourceType::File)
		{
			// JSONファイルと同じディレクトリからの相対パスとして扱う
			const String fontPath = FileSystem::PathAppend(FileSystem::ParentPath(path), sourceJson[U"path"].getString());

			if (sourceJson.contains(U"faceIndex"))
			{
				const size_t faceIndex = sourceJson[U"faceIndex"].get<size_t>();
				FontAsset::Register(fontAssetName, method, fontSize, fontPath, faceIndex, style);
			}
			else
			{
				FontAsset::Register(fontAssetName, method, fontSize, fontPath, style);
			}
		}
		else if (*sourceType == FontSourceType::Typeface)
		{
			const String typefaceStr = sourceJson[U"typeface"].getString();
			const TypefaceType typefaceType = StringToEnumOpt<TypefaceType>(typefaceStr).value_or(TypefaceType::Regular);
			const Typeface typeface = ConvertToS3dTypeface(typefaceType);
			FontAsset::Register(fontAssetName, method, fontSize, typeface, style);
		}

		m_registeredFontAssetNames.push_back(fontAssetName);
	}

	void CustomAssetLoader::loadPixelShaderAssetsFromDirectory(const FilePath& directory)
	{
		if (!FileSystem::IsDirectory(directory))
		{
			return;
		}
		for (const FilePath& path : FileSystem::DirectoryContents(directory, Recursive::Yes))
		{
			if (FileSystem::Extension(path) == U"json")
			{
				loadPixelShaderAssetFromFile(path);
			}
		}
	}

	void CustomAssetLoader::loadPixelShaderAssetFromFile(const FilePath& path)
	{
		const JSON json = JSON::Load(path);
		if (not json)
		{
			Logger << U"[NocoEditor warning] Failed to load pixel shader asset JSON: " << path;
			return;
		}

		const String pixelShaderAssetName = json[U"pixelShaderAssetName"].getString();
		if (pixelShaderAssetName.isEmpty())
		{
			Logger << U"[NocoEditor warning] pixelShaderAssetName is empty in: " << path;
			return;
		}

		if (PixelShaderAsset::IsRegistered(pixelShaderAssetName))
		{
			// 優先度の高いディレクトリで同名のアセットが登録済みの場合はスキップ
			Logger << U"[NocoEditor warning] Pixel shader asset '{}' is already registered. Skipping: {}"_fmt(pixelShaderAssetName, path);
			return;
		}

		const FilePath parentDir = FileSystem::ParentPath(path);
		Optional<HLSL> hlsl;
		Optional<GLSL> glsl;
		Optional<MSL> msl;
		Optional<ESSL> essl;
		Optional<WGSL> wgsl;

		if (json.contains(U"hlsl"))
		{
			const JSON& h = json[U"hlsl"];
			hlsl = HLSL{
				FilePath{ FileSystem::PathAppend(parentDir, h[U"path"].getString()) },
				h.contains(U"entryPoint") ? h[U"entryPoint"].getString() : String{ U"PS" },
			};
		}

		if (json.contains(U"glsl"))
		{
			const JSON& g = json[U"glsl"];
			Array<ConstantBufferBinding> bindings;
			if (g.contains(U"bindings"))
			{
				for (const auto& b : g[U"bindings"].arrayView())
				{
					bindings.push_back(ConstantBufferBinding{
						b[U"name"].getString(),
						b[U"index"].get<uint32>(),
					});
				}
			}
			glsl = GLSL{
				FilePath{ FileSystem::PathAppend(parentDir, g[U"path"].getString()) },
				bindings,
			};
		}

		if (json.contains(U"msl"))
		{
			const JSON& m = json[U"msl"];
			if (m.contains(U"path"))
			{
				msl = MSL{
					FilePath{ FileSystem::PathAppend(parentDir, m[U"path"].getString()) },
					m[U"entryPoint"].getString(),
				};
			}
			else
			{
				msl = MSL{ m[U"entryPoint"].getString() };
			}
		}

		if (json.contains(U"essl"))
		{
			const JSON& e = json[U"essl"];
			Array<ConstantBufferBinding> bindings;
			if (e.contains(U"bindings"))
			{
				for (const auto& b : e[U"bindings"].arrayView())
				{
					bindings.push_back(ConstantBufferBinding{
						b[U"name"].getString(),
						b[U"index"].get<uint32>(),
					});
				}
			}
			essl = ESSL{
				FilePath{ FileSystem::PathAppend(parentDir, e[U"path"].getString()) },
				bindings,
			};
		}

		if (json.contains(U"wgsl"))
		{
			const JSON& w = json[U"wgsl"];
			Array<ConstantBufferBinding> bindings;
			if (w.contains(U"bindings"))
			{
				for (const auto& b : w[U"bindings"].arrayView())
				{
					bindings.push_back(ConstantBufferBinding{
						b[U"name"].getString(),
						b[U"index"].get<uint32>(),
					});
				}
			}
			wgsl = WGSL{
				FilePath{ FileSystem::PathAppend(parentDir, w[U"path"].getString()) },
				bindings,
			};
		}

		ShaderGroup shaderGroup;
		if (hlsl)
		{
			shaderGroup = shaderGroup | *hlsl;
		}
		if (glsl)
		{
			shaderGroup = shaderGroup | *glsl;
		}
		if (msl)
		{
			shaderGroup = shaderGroup | *msl;
		}
		if (essl)
		{
			shaderGroup = shaderGroup | *essl;
		}
		if (wgsl)
		{
			shaderGroup = shaderGroup | *wgsl;
		}

		PixelShaderAsset::Register(pixelShaderAssetName, shaderGroup);
		m_registeredPixelShaderAssetNames.push_back(pixelShaderAssetName);
	}
}
