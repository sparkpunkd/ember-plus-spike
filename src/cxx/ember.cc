#define LIBEMBER_HEADER_ONLY
#include "ember/Ember.hpp"

#include <assert.h>
#include <node_api.h>

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }
#define CHECK_STATUS if (checkStatus(env, status, __FILE__, __LINE__ - 1) != napi_ok) { return nullptr; }

napi_status checkStatus(napi_env env, napi_status status,
  const char* file, uint32_t line) {

  napi_status infoStatus, throwStatus;
  const napi_extended_error_info *errorInfo;

  if (status == napi_ok) {
    // printf("Received status OK.\n");
    return status;
  }

  infoStatus = napi_get_last_error_info(env, &errorInfo);
  assert(infoStatus == napi_ok);
  printf("NAPI error in file %s on line %i. Error %i: %s\n", file, line,
    errorInfo->error_code, errorInfo->error_message);

  if (status == napi_pending_exception) {
    printf("NAPI pending exception. Engine error code: %i\n", errorInfo->engine_error_code);
    return status;
  }

  char errorCode[20];
  sprintf(errorCode, "%d", errorInfo->error_code);
  throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message);
  assert(throwStatus == napi_ok);

  return napi_pending_exception; // Expect to be cast to void
}


napi_value testEmber (napi_env env, napi_callback_info info) {
	napi_status status;
	napi_value result;

	libember::glow::GlowRootElementCollection* root = libember::glow::GlowRootElementCollection::create();
	status = napi_create_external(env, root, nullptr, nullptr, &result);
	CHECK_STATUS;

	return result;
};

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;

  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("testEmber", testEmber)
	};
  status = napi_define_properties(env, exports, 1, desc);
	CHECK_STATUS;

  return exports;
};

NAPI_MODULE(ember, Init)
