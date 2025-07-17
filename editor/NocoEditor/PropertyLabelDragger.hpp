#pragma once
#include <Siv3D.hpp>
#include <NocoUI.hpp>

namespace noco::editor
{
	class PropertyLabelDragger : public ComponentBase
	{
	private:
		std::function<void(double)> m_fnSetValue;
		std::function<double()> m_fnGetValue;
		std::function<void()> m_fnOnDragStart;
		std::function<void()> m_fnOnDragEnd;
		double m_dragStartValue = 0.0;
		Vec2 m_dragStartPos = Vec2::Zero();
		bool m_isDragging = false;
		double m_step = 1.0;
		double m_minValue = -std::numeric_limits<double>::max();
		double m_maxValue = std::numeric_limits<double>::max();
		
	public:
		PropertyLabelDragger(
			std::function<void(double)> fnSetValue,
			std::function<double()> fnGetValue,
			double step = 1.0,
			double minValue = -std::numeric_limits<double>::max(),
			double maxValue = std::numeric_limits<double>::max(),
			std::function<void()> fnOnDragStart = nullptr,
			std::function<void()> fnOnDragEnd = nullptr)
			: ComponentBase{ {} }
			, m_fnSetValue{ std::move(fnSetValue) }
			, m_fnGetValue{ std::move(fnGetValue) }
			, m_fnOnDragStart{ std::move(fnOnDragStart) }
			, m_fnOnDragEnd{ std::move(fnOnDragEnd) }
			, m_step{ step }
			, m_minValue{ minValue }
			, m_maxValue{ maxValue }
		{
		}
		
		void update(const std::shared_ptr<Node>& node) override;
		
		void setSensitivity(double step) { m_step = step; }
		double getSensitivity() const { return m_step; }
		
		void setMinValue(double minValue) { m_minValue = minValue; }
		double getMinValue() const { return m_minValue; }
		
		void setMaxValue(double maxValue) { m_maxValue = maxValue; }
		double getMaxValue() const { return m_maxValue; }
		
		bool isDragging() const { return m_isDragging; }
	};
}
