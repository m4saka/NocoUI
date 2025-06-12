#pragma once
#include <Siv3D.hpp>

namespace noco
{
	class Node;
	
	// フォーカス可能なコンポーネントのインターフェース
	class IFocusable
	{
	public:
		virtual ~IFocusable() = default;
		
		// フォーカスを取得したときに呼ばれる
		virtual void focus(const std::shared_ptr<Node>& node) = 0;
		
		// フォーカスを失ったときに呼ばれる
		virtual void blur(const std::shared_ptr<Node>& node) = 0;
		
		// フォーカス可能かどうかを返す（オプション）
		[[nodiscard]]
		virtual bool canFocus() const
		{
			return true;
		}
	};
}