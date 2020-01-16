#define LIBEMBER_HEADER_ONLY
#include "ember/Ember.hpp"
#include "s101/S101.hpp"

#include <assert.h>
#include <node_api.h>

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }
#define CHECK_STATUS if (checkStatus(env, status, __FILE__, __LINE__ - 1) != napi_ok) { return nullptr; }
#define PASS_STATUS if (status != napi_ok) return status

#define NAPI_THROW_ERROR(msg) { \
  char errorMsg[256]; \
  sprintf(errorMsg, "%s", msg); \
  napi_throw_error(env, nullptr, errorMsg); \
  return nullptr; \
}

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


napi_value testEncode (napi_env env, napi_callback_info info) {
	napi_status status;
	napi_value result, value;

	libember::glow::GlowRootElementCollection* root = libember::glow::GlowRootElementCollection::create();
	libember::glow::GlowCommand* getDir = new libember::glow::GlowCommand(root, libember::glow::CommandType::GetDirectory);

	status = napi_create_object(env, &result);
	CHECK_STATUS;

	status = napi_create_string_utf8(env, "GlowCommand", NAPI_AUTO_LENGTH, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "type", value);
	CHECK_STATUS;

	// TODO Nested tree
	status = napi_create_string_utf8(env, "GetDirectory", NAPI_AUTO_LENGTH, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "command", value);
	CHECK_STATUS;

	// TODO destructor
	status = napi_create_external(env, getDir, nullptr, nullptr, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "_command", value);
	CHECK_STATUS;

	status = napi_create_int32(env, getDir->encodedLength(), &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "encodedLength", value);
	CHECK_STATUS;

	libember::util::OctetStream fred(1024);
	getDir->encode(fred);

	char* gdd = new char[fred.size() + 1];
	std::copy(fred.begin(), fred.end(), gdd);

	status = napi_create_buffer_copy(env, fred.size(), gdd, nullptr, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "asn1", value);
	CHECK_STATUS;

	auto encoder = libs101::StreamEncoder<unsigned char>();
	auto const version = libember::glow::GlowDtd::version();
	auto const isEmpty = fred.begin() == fred.end();
	auto const flags = (unsigned char)(
					(false ? libs101::PackageFlag::FirstPackage : 0) |
					(true ? libs101::PackageFlag::LastPackage : 0) |
					(isEmpty ? libs101::PackageFlag::EmptyPackage : 0)
			);

	encoder.encode(0x00);                       // Slot
	encoder.encode(libs101::MessageType::EmBER);// Message type
	encoder.encode(libs101::CommandType::EmBER);// Ember Command
	encoder.encode(0x01);                       // Version
	encoder.encode(flags);                      // Flags
	encoder.encode(libs101::Dtd::Glow);         // Glow Dtd
	encoder.encode(0x02);                       // App bytes low
	encoder.encode((version >> 0) & 0xFF);      // App specific, minor revision
	encoder.encode((version >> 8) & 0xFF);      // App specific, major revision
	encoder.encode(fred.begin(), fred.end());
	encoder.finish();

	char* res = new char[encoder.size() + 1];
	std::copy(encoder.begin(), encoder.end(), res);

	status = napi_create_buffer_copy(env, encoder.size() + 1, res, nullptr, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "encodedData", value);
	CHECK_STATUS;

	return result;
};

napi_status treeToObject (napi_env env, libember::glow::GlowContainer const* glow, napi_value* result) {
	napi_status status;
	napi_value value, kids;

	printf("Well hello %p\n", glow);
	printf("Look at my tag %i\n", glow->typeTag().number());

	using libember::glow::GlowType;
	switch(glow->typeTag().number()) {
		case GlowType::Command:
			//handleCommand(env, static_cast<libember::glow::GlowCommand const*>(glow), pathToOid());
			const libember::glow::GlowCommand* command = static_cast<libember::glow::GlowCommand const*>(glow);

			status = napi_create_string_utf8(env, "GlowCommand", NAPI_AUTO_LENGTH, &value);
			// CHECK_STATUS;
			status = napi_set_named_property(env, *result, "type", value);
			// CHECK_STATUS;

			status = napi_create_int32(env, (int32_t) command->number().value(), &value);
			// CHECK_STATUS;
			status = napi_set_named_property(env, *result, "number", value);
			// CHECK_STATUS;

			status = napi_create_array(env, &kids);
			uint32_t index = 0;
			printf("Command size %i\n", command->size());
			for ( auto first = command->begin() ; first != command->end() ; first++ ) {
				status = napi_create_object(env, &value);
				auto glowkid = static_cast<libember::glow::GlowContainer const*>(std::addressof(*first));
				if (glowkid != nullptr) {
					treeToObject(env, glowkid, &value);
				} else {
					printf("Null in house %p\n", std::addressof(*first));
				}
				status = napi_set_element(env, kids, index++, value);
			}
			status = napi_set_named_property(env, *result, "children", kids);

			break;
		defualt:
			printf("Not handling type tag %i\n", glow->typeTag().number());
			break;
	}
	return napi_ok;
}

napi_value tree (napi_env env, napi_callback_info info) {
	napi_status status;
	napi_value result, value;

	libember::glow::GlowRootElementCollection* root = libember::glow::GlowRootElementCollection::create();
	libember::glow::GlowNode* device = new libember::glow::GlowNode(1);
	libember::glow::GlowNode* network = new libember::glow::GlowNode(3);
	libember::glow::GlowCommand* command = new libember::glow::GlowCommand(libember::glow::CommandType::GetDirectory);

	root->insert(root->end(), device);
	device->children()->insert(device->children()->end(), network);
	network->children()->insert(network->children()->end(), command);

	status = napi_create_object(env, &result);
	CHECK_STATUS;

	status = treeToObject(env, command, &result);
	CHECK_STATUS;

	// Do something to seialize the tree
	return result;
}

void decodeCallback(
	std::vector<unsigned char>::const_iterator first,
	std::vector<unsigned char>::const_iterator last) {
	printf("Decode callback has been called - length %i.\n", last - first);
	std::vector<unsigned char>::const_iterator begin = first;
	for ( auto it = first ; first != last ; first++ ) {
		printf("%02X ", *first);
	}
	printf("\n");
	first = begin;
	libember::util::StreamBuffer<unsigned char> buf;
	buf.append(first, last);

	printf("Appended to buffer %i\n", buf.size());
	buf.consume(9);
	printf("Buffer after consume %i\n", buf.size());
	libember::dom::DomReader reader;

	printf("Created DOM reader\n");
	libember::dom::Node* noddy = reader.decodeTree(buf, libember::glow::GlowNodeFactory::getFactory());
	printf("I'm a node with encoded Length %i.\n", noddy->encodedLength());
}

napi_value testDecode (napi_env env, napi_callback_info info) {
	napi_status status;
	napi_value result, value;
	bool testResult;
	void* data;
	size_t dataLength;

	printf("Running testDecode\n");
	napi_value argv[1];
	size_t argc = 1;
	status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
	CHECK_STATUS;

	if (argc != 1) {
		NAPI_THROW_ERROR("Test decoder must be provided with a single value - a buffer.");
	}

	status = napi_is_buffer(env, argv[0], &testResult);
	CHECK_STATUS;
	if (!testResult) {
		NAPI_THROW_ERROR("Test decoder must be provided with a buffer.");
	}

	status = napi_get_buffer_info(env, argv[0], &data, &dataLength);
	CHECK_STATUS;

	char* datac = (char*) data;
	std::vector<unsigned char> it(datac, datac + dataLength);

	libs101::StreamDecoder<unsigned char> decoder;
	decoder.read(it.begin(), it.end(), decodeCallback);

	// Decode ASN.1 to Glow object
	// Convert to something Javascripty

	return nullptr;
}

napi_value version (napi_env env, napi_callback_info info) {
	napi_status status;
	napi_value result, value;

	status = napi_create_object(env, &result);
	CHECK_STATUS;

	status = napi_create_string_utf8(env, libember::versionString, NAPI_AUTO_LENGTH, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "libember", value);
	CHECK_STATUS;

	auto const version = libember::glow::GlowDtd::version();
	int factor = ((version >> 8) & 0xFF) > 9 ? 100 : 10;
	status = napi_create_double(env, ((version >> 0) & 0xFF) + (double) ((version >> 8) & 0xFF) / factor, &value);
	CHECK_STATUS;
	status = napi_set_named_property(env, result, "glow", value);
	CHECK_STATUS;

	return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;

  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("testEncode", testEncode),
		DECLARE_NAPI_METHOD("tree", tree),
		DECLARE_NAPI_METHOD("version", version),
		DECLARE_NAPI_METHOD("testDecode", testDecode)
	};
  status = napi_define_properties(env, exports, 4, desc);
	CHECK_STATUS;

  return exports;
};

NAPI_MODULE(ember, Init)
