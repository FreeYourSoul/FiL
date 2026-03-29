// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FIL_EXTRACT_MEMBER_TYPE_HH
#define FIL_EXTRACT_MEMBER_TYPE_HH

namespace fil {

//! Helper trait to extract class type from pointer-to-member
template<typename PointerToMember>
struct extract_class;

// Specialization for data members
template<typename T, typename MemberType>
struct extract_class<MemberType T::*> {
    using type = T;
};

//! Specialization for member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...)> {
    using type = T;
};

//! Specialization for member & functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) &> {
    using type = T;
};

//! Specialization for member && functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) &&> {
    using type = T;
};

//! Specialization for member noexcept functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) noexcept> {
    using type = T;
};

//! Specialization for member & noexcept functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) & noexcept> {
    using type = T;
};

//! Specialization for member && noexcept functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) && noexcept> {
    using type = T;
};

//! Const member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const> {
    using type = T;
};

//! Const & member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const&> {
    using type = T;
};

//! Const && member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const&&> {
    using type = T;
};

//! Const noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const noexcept> {
    using type = T;
};

//! Const & noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const & noexcept> {
    using type = T;
};

//! Const && noexcept  member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const && noexcept> {
    using type = T;
};

//! Volatile member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile> {
    using type = T;
};

//! Volatile & member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile&> {
    using type = T;
};

//! Volatile && member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile&&> {
    using type = T;
};

//! Const volatile member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile> {
    using type = T;
};

//! Const volatile & member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile&> {
    using type = T;
};

//! Const volatile && member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile&&> {
    using type = T;
};
//! Volatile noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile noexcept> {
    using type = T;
};

//! Volatile & noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile & noexcept> {
    using type = T;
};

//! Volatile && noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) volatile && noexcept> {
    using type = T;
};

//! Const volatile member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile noexcept> {
    using type = T;
};

//! Const volatile & noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile & noexcept> {
    using type = T;
};

//! Const volatile && noexcept member functions
template<typename T, typename ReturnType, typename... Args>
struct extract_class<ReturnType (T::*)(Args...) const volatile && noexcept> {
    using type = T;
};

//! Convenient alias template to get the type directly
template<typename PointerToMember>
using extract_class_t = extract_class<PointerToMember>::type;

} // namespace fil

#endif // FIL_EXTRACT_MEMBER_TYPE_HH
