#pragma once
#include <Siv3D.hpp>

namespace noco
{
	/// @brief リッチテキストのタグとして解釈されないよう'<'と'>'をエスケープした文字列を返す(外部由来の文字列をリッチテキストに埋め込む場合に使用)
	[[nodiscard]]
	inline String EscapeRichText(StringView text)
	{
		String result;
		result.reserve(text.size());
		for (const char32 ch : text)
		{
			if (ch == U'<')
			{
				result += U"<lt>";
			}
			else if (ch == U'>')
			{
				result += U"<gt>";
			}
			else
			{
				result.push_back(ch);
			}
		}
		return result;
	}
}
