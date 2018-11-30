/*
 * Intrinsic functions available to WASM filter modules.
 */
#include <string_view>

#include "envoy_wasm_intrinsics.h"


class WasmData {
public:
  WasmData(const char* data, size_t size) : data_(data), size_(size) {}
  ~WasmData() { ::free((void*)data_); }
  std::string_view view() { return {data_, size_}; }

  WasmData& operator=(const WasmData&) = delete;
  WasmData(const WasmData&) = delete;

private:
  const char* data_;
  size_t size_;
};
typedef std::unique_ptr<WasmData> WasmDataPtr;

enum class HeaderType : int { Header = 0, Trailer = 1 };
enum class FilterHeadersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterTrailersStatus : int { Continue = 0, StopIteration = 1 };
enum class FilterDataStatus : int { Continue = 0, StopIterationAndBuffer = 1,
  StopIterationAndWatermark = 2, StopIterationNoBuffer = 3 };

// Calls from WASM to Envoy.
extern "C" void envoy_getHeader(HeaderType type, const char* key_ptr, size_t key_size,
                                 const char** value_ptr, size_t* value_size);

inline WasmDataPtr getHeader(HeaderType type, std::string_view key) {
  const char* value_ptr = nullptr;
  size_t value_size = 0;
  envoy_getHeader(type, key.data(), key.size(), &value_ptr, &value_size);
  return std::make_unique<WasmData>(value_ptr, value_size);
}

extern "C" void envoy_replaceHeader(HeaderType type, const char* key_ptr, size_t key_size,
                                     const char* value_ptr, size_t value_size);

inline void replaceHeader(HeaderType type, std::string_view key, std::string_view value) {
  envoy_replaceHeader(type, key.data(), key.size(), value.data(), value.size());
}

extern "C" void envoy_removeHeader(HeaderType type, const char* key_ptr, size_t key_size);

inline void removeHeader(HeaderType type, std::string_view key) {
  envoy_removeHeader(type, key.data(), key.size());
}

extern "C" void envoy_getBodyBufferBytes(uint32_t start, uint32_t length, const char** ptr, size_t* size);

inline WasmDataPtr getBodyBufferBytes(size_t start, size_t length) {
  const char* ptr = nullptr;
  size_t size = 0;
  envoy_getBodyBufferBytes(start, length, &ptr, &size);
  return std::make_unique<WasmData>(ptr, size);
}

// Calls from Envoy to WASM.
extern "C" int main();

extern "C" FilterHeadersStatus onStart(int32_t context_id);
extern "C" FilterDataStatus onBody(int32_t context_id, uint32_t body_buffer_length, uint32_t end_of_stream);
extern "C" FilterTrailersStatus onTrailers(int32_t context_id);
extern "C" void onDestroy(int32_t context_id);
