#ifndef FIL_SHALLOW_COPY_HH
#define FIL_SHALLOW_COPY_HH

namespace fil {

/**
 * @brief copy functor called in some cases of the fil library.
 * It is called when ensured that the previous instance of the object is still alive for the whole duration of the new object.
 * This allows for shallow copies that would normally be unsafe.
 * To enable this, a specialization of this template function must be made with the type to shallow copy
 *
 * @tparam T type to shallow copy
 * @param object to be shallowly copied
 * @return a shallow copy of the object if a specialization is found, otherwise a normal copy is returned
 */
template<typename T>
struct shallow_copy {
    static constexpr auto copy(const T& object) { return object; }
    static constexpr auto assign(T& object, T&& other) { object = std::move(other); }
};

} // namespace fil

#endif // FIL_SHALLOW_COPY_HH
