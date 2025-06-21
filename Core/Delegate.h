#pragma

namespace cgs::core
{
	template <typename T>
	class Delegate final {};

	template<typename ReturnType, typename... Args>
	class Delegate<ReturnType(Args...)> final
	{
	public:
		using FunctionType = ReturnType(*)(Args...) noexcept; // Type alias for a function pointer
		template<class ClassType>
		using MemberFunctionPtr = ReturnType(ClassType::*)(Args...) noexcept; // Type alias for a member function pointer with class type
		template<class ClassType>
		using ConstMemberFunctionPtr = ReturnType(ClassType::*)(Args...) const noexcept; // Type alias for a const member function pointer with class type
		using ProxyFunctionType = ReturnType(*)(void*, Args...) noexcept;

		template<FunctionType>
		static ReturnType FunctionProxy(void*, Args... args) noexcept
		{
			return Function(std::forward<Args>(args)...); // Call the function with the provided arguments
		}

		template<class ClassType, MemberFunctionPtr<ClassType> MemberFunction>
		static ReturnType MethodProxy(void* instance, Args... args) noexcept
		{
			return (static_cast<ClassType*>(instance)->*MemberFunction)(std::forward<Args>(args)...); // Call the member function on the instance with the provided arguments
		}

		template<class ClassType, ConstMemberFunctionPtr<ClassType> MemberFunction>
		static ReturnType ConstMethodProxy(const void* instance, Args... args) noexcept
		{
			return (static_cast<const ClassType*>(instance)->*MemberFunction)(std::forward<Args>(args)...); // Call the const member function on the instance with the provided arguments
		}

	public:
		Delegate() noexcept = default; // Default constructor
		Delegate(const Delegate&) = delete; // Disable copy constructor
		Delegate(Delegate&&) noexcept = default; // Default move constructor
		~Delegate() noexcept = default; // Default destructor
		Delegate& operator=(const Delegate&) = delete; // Disable copy assignment operator
		Delegate& operator=(Delegate&&) noexcept = default; // Default move assignment operator

		template<FunctionType function>
		void Bind() noexcept
		{
			mFunction = &FunctionProxy<function>; // Bind a free function
			mInstance = nullptr; // No instance for free functions
		}

		template<class ClassType, MemberFunctionPtr<ClassType> MemberFunction>
		void Bind(ClassType* instance) noexcept
		{
			mFunction = &MethodProxy<ClassType, MemberFunction>; // Bind a member function
			mInstance = instance; // Store the instance for member functions
		}

		template<class ClassType, ConstMemberFunctionPtr<ClassType> MemberFunction>
		void Bind(const ClassType* instance) noexcept
		{
			mFunction = &ConstMethodProxy<ClassType, MemberFunction>; // Bind a const member function
			mInstance = const_cast<ClassType*>(instance); // Store the instance for const member functions
		}

		ReturnType Invoke(Args... args) const noexcept
		{
			if (mFunction)
			{
				return mFunction(mInstance, std::forward<Args>(args)...); // Call the bound function or method with the provided arguments
			}
			return ReturnType(); // Return a default value if no function is bound
		}

	private:
		ProxyFunctionType mFunction; // Pointer to the bound function or method
		void* mInstance; // Pointer to the instance for member functions, nullptr for free functions
	};
	
  //  class Delegate final
  //  {
  //  public:
  //      struct Wrapper final
  //      {
  //          virtual int32_t Invoke(const int32_t) = 0;
  //      };
  //  
  //      struct FunctionWrapper final : public Wrapper
  //      {
  //      public:
  //          using FunctionType = int32_t (*)(const int32_t);
  //  
  //      public:
  //          CGS_INLINE FunctionWrapper(FunctionType function) noexcept
  //              : mFunction(function) {}
  //  
  //          CGS_INLINE int32_t Invoke(const int32_t value) override
  //          {
  //              return mFunction(value);
  //          }

  //      private:
  //          FunctionType mFunction; // Pointer to the function to be invoked
  //      };

  //      template<typename T>
  //      struct MemberFunctionWrapper final : public Wrapper
  //      {
  //      public:
  //          using MemberFunctionType = int32_t (T::*)(const int32_t);
  //      public:
  //          CGS_INLINE MemberFunctionWrapper(T* instance, MemberFunctionType memberFunction) noexcept
  //              : mInstance(instance), mMemberFunction(memberFunction) {}

  //          CGS_INLINE int32_t Invoke(const int32_t value) override
  //          {
  //              return (mInstance->*mMemberFunction)(value);
  //          }

  //      private:
  //          T* mInstance; // Pointer to the instance of the class containing the member function
  //          MemberFunctionType mMemberFunction; // Pointer to the member function to be invoked
  //      };
  //  
  //  public:
  //      Delegate() = delete; // Prevent instantiation of the Delegate class
  //      Delegate(const Delegate&) = delete; // Disable copy constructor
  //      Delegate(Delegate&&) noexcept = default; // Disable move constructor
  //      ~Delegate() noexcept = default; // Default destructor

		//Delegate& operator=(const Delegate&) = delete; // Disable copy assignment operator
		//Delegate& operator=(Delegate&&) noexcept = default; // Disable move assignment operator

  //      void Bind(int32_t (*function)(const int32_t)) noexcept
  //      {
  //          mWrapper = std::make_unique<FunctionWrapper>(function);
  //      }

  //      template<typename T>
  //      void Bind(T* instance, int32_t (T::*memberFunction)(const int32_t)) noexcept
  //      {
  //          mWrapper = std::make_unique<MemberFunctionWrapper<T>>(instance, memberFunction);
  //      }

  //      int32_t Invoke(const int32_t value) const noexcept
  //      {
  //          if (mWrapper)
  //          {
  //              return mWrapper->Invoke(value);
  //          }
  //          return 0; // Return a default value if no function is bound
  //      }
  //  
  //  private:
  //      std::unique_ptr<Wrapper> mWrapper; // Unique pointer to the wrapper that holds the function or member function
  //  };
}   // namespace cgs::core