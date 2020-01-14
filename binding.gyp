{
  "targets": [{
    "target_name" : "ember",
    "sources" : [
	  "src/cxx/ember.cc",
	],
    "conditions": [
      ['OS=="win"', {
        "configurations": {
          "Release": {
            "msvs_settings": {
              "VCCLCompilerTool": {
                "RuntimeTypeInfo": "true",
				"ExceptionHandling": 1
              }
            }
          }
        },
        "include_dirs" : [
          "include"
        ]
    }],
	  ['OS!="win"', {
	    "defines": [
	      "__STDC_CONSTANT_MACROS", "OMNI_UNLOADABLE_STUBS"
	    ],
        "include_dirs" : [
          "include"
        ],
	    "cflags_cc": [
	      "-std=c++11",
	      "-fexceptions"
	    ]
	  }]
	]
  }]
}
