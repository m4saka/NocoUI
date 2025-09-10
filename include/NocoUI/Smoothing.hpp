#pragma once
#include <Siv3D.hpp>

namespace noco
{
	template <class T>
	concept HasSmoothDamp = requires(T t)
	{
		{ Math::SmoothDamp(t, t, t, 0.0) } -> std::convertible_to<T>;
	};

	template <class T>
	concept HasSmoothDampInner = requires(T t)
	{
		{ T::SmoothDamp(t, t, t, 0.0) } -> std::convertible_to<T>;
	};

	template <class T>
	class Smoothing
	{
		static_assert(HasSmoothDamp<T> || HasSmoothDampInner<T>, "T must have Math::SmoothDamp");

	private:
		/*NonSerialized*/ T m_currentValue;
		/*NonSerialized*/ T m_velocity;

	public:
		explicit Smoothing(const T& initialValue, const T& initialVelocity = T{})
			: m_currentValue{ initialValue }
			, m_velocity{ initialVelocity }
		{
		}

		void update(const T& targetValue, double smoothTime, double deltaTime)
		{
			if (smoothTime <= 0.0)
			{
				m_currentValue = targetValue;
				return;
			}
			if constexpr (HasSmoothDamp<T>)
			{
				m_currentValue = Math::SmoothDamp(m_currentValue, targetValue, m_velocity, smoothTime, unspecified, deltaTime);
			}
			else if constexpr (HasSmoothDampInner<T>)
			{
				m_currentValue = T::SmoothDamp(m_currentValue, targetValue, m_velocity, smoothTime, unspecified, deltaTime);
			}
		}

		[[nodiscard]]
		const T& currentValue() const
		{
			return m_currentValue;
		}

		void setCurrentValue(const T& value)
		{
			m_currentValue = value;
			m_velocity = T{};
		}
	};
}
