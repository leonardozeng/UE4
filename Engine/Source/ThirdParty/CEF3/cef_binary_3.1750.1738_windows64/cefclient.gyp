# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
    'framework_name': 'Chromium Embedded Framework',
    'linux_use_gold_binary': 0,
    'linux_use_gold_flags': 0,
    'conditions': [
      [ 'OS=="mac"', {
        # Don't use clang with CEF binary releases due to Chromium tree structure dependency.
        'clang': 0,
      }],
      ['sysroot!=""', {
        'pkg-config': './pkg-config-wrapper "<(sysroot)" "<(target_arch)"',
      }, {
        'pkg-config': 'pkg-config'
      }],
      [ 'OS=="win"', {
        'multi_threaded_dll%': 0,
      }],
    ]
  },
  'includes': [
    # Bring in the source file lists for cefclient.
    'cef_paths2.gypi',
  ],
  'targets': [
    {
      'target_name': 'cefclient',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': '6617FED9-C5D4-4907-BF55-A90062A6683F',
      'dependencies': [
        'libcef_dll_wrapper',
      ],
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_wrapper)',
        '<@(cefclient_sources_common)',
      ],
      'mac_bundle_resources': [
        '<@(cefclient_bundle_resources_mac)',
      ],
      'mac_bundle_resources!': [
        # TODO(mark): Come up with a fancier way to do this (mac_info_plist?)
        # that automatically sets the correct INFOPLIST_FILE setting and adds
        # the file to a source group.
        'cefclient/mac/Info.plist',
      ],
      'xcode_settings': {
        'INFOPLIST_FILE': 'cefclient/mac/Info.plist',
        # Target build path.
        'SYMROOT': 'xcodebuild',
      },
      'conditions': [
        ['OS=="win"', {
          'variables': {
            'win_exe_compatibility_manifest': 'cefclient/compatibility.manifest',
          },
          'actions': [
            {
              'action_name': 'copy_resources',
              'msvs_cygwin_shell': 0,
              'inputs': [],
              'outputs': [
                '<(PRODUCT_DIR)/copy_resources.stamp',
              ],
              'action': [
                'xcopy /efy',
                'Resources\*',
                '$(OutDir)',
              ],
            },
            {
              'action_name': 'copy_libraries',
              'msvs_cygwin_shell': 0,
              'inputs': [],
              'outputs': [
                '<(PRODUCT_DIR)/copy_resources.stamp',
              ],
              'action': [
                'xcopy /efy',
                '$(ConfigurationName)\*.dll',
                '$(OutDir)',
              ],
            },
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
            'VCManifestTool': {
              'AdditionalManifestFiles': [
                'cefclient/cefclient.exe.manifest',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-lopengl32.lib',
              '-lglu32.lib',
              '-l$(ConfigurationName)/libcef.lib',
            ],
          },
          'library_dirs': [
            # Needed to find cef_sandbox.lib using #pragma comment(lib, ...).
            '$(ConfigurationName)',
          ],
          'sources': [
            '<@(includes_win)',
            '<@(cefclient_sources_win)',
          ],
        }],
        [ 'OS=="win" and multi_threaded_dll', {
          'configurations': {
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 3,
                  'WarnAsError': 'false',
                },
              },
            },
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 2,
                  'WarnAsError': 'false',
                },
              },
            }
          }
        }],
        [ 'OS=="mac"', {
          'product_name': 'cefclient',
          'dependencies': [
            'cefclient_helper_app',
          ],
          'copies': [
            {
              # Add libraries and helper app.
              'destination': '<(PRODUCT_DIR)/cefclient.app/Contents/Frameworks',
              'files': [
                '$(CONFIGURATION)/libplugin_carbon_interpose.dylib',
                '<(PRODUCT_DIR)/cefclient Helper.app',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Add framework',
              'action': [
                'cp',
                '-Rf',
                '${CONFIGURATION}/<(framework_name).framework',
                '${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/'
              ],
            },
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/<(framework_name)',
                '@executable_path/../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
            {
              # This postbuid step is responsible for creating the following
              # helpers:
              #
              # cefclient Helper EH.app and cefclient Helper NP.app are created
              # from cefclient Helper.app.
              #
              # The EH helper is marked for an executable heap. The NP helper
              # is marked for no PIE (ASLR).
              'postbuild_name': 'Make More Helpers',
              'action': [
                'tools/make_more_helpers.sh',
                'Frameworks',
                'cefclient',
              ],
            },
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(cefclient_sources_mac)',
          ],
        }],
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/files',
              'files': [
                '<@(cefclient_bundle_resources_linux)',
              ],
            },
            {
              'destination': '<(PRODUCT_DIR)/',
              'files': [
                'Resources/cef.pak',
                'Resources/devtools_resources.pak',
                'Resources/locales/',
                '$(BUILDTYPE)/chrome-sandbox',
                '$(BUILDTYPE)/libcef.so',
                '$(BUILDTYPE)/libffmpegsumo.so',
              ],
            },
          ],
          'dependencies': [
            'gtk',
            'gtkglext',
          ],
          'link_settings': {
            'ldflags': [
              # Look for libcef.so in the current directory. Path can also be
              # specified using the LD_LIBRARY_PATH environment variable.
              '-Wl,-rpath,.',
            ],
            'libraries': [
              "$(BUILDTYPE)/libcef.so",
            ],
          },
          'sources': [
            '<@(includes_linux)',
            '<@(cefclient_sources_linux)',
          ],
        }],
      ],
    },
    {
      'target_name': 'cefsimple',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': '5390D142-473F-49A0-BC5E-5F6C609EEDB6',
      'dependencies': [
        'libcef_dll_wrapper',
      ],
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_wrapper)',
        '<@(cefsimple_sources_common)',
      ],
      'mac_bundle_resources': [
        '<@(cefsimple_bundle_resources_mac)',
      ],
      'mac_bundle_resources!': [
        # TODO(mark): Come up with a fancier way to do this (mac_info_plist?)
        # that automatically sets the correct INFOPLIST_FILE setting and adds
        # the file to a source group.
        'cefsimple/mac/Info.plist',
      ],
      'xcode_settings': {
        'INFOPLIST_FILE': 'cefsimple/mac/Info.plist',
        # Target build path.
        'SYMROOT': 'xcodebuild',
      },
      'conditions': [
        ['OS=="win"', {
          'variables': {
            'win_exe_compatibility_manifest': 'cefsimple/compatibility.manifest',
          },
          'actions': [
            {
              'action_name': 'copy_resources',
              'msvs_cygwin_shell': 0,
              'inputs': [],
              'outputs': [
                '<(PRODUCT_DIR)/copy_resources.stamp',
              ],
              'action': [
                'xcopy /efy',
                'Resources\*',
                '$(OutDir)',
              ],
            },
            {
              'action_name': 'copy_libraries',
              'msvs_cygwin_shell': 0,
              'inputs': [],
              'outputs': [
                '<(PRODUCT_DIR)/copy_resources.stamp',
              ],
              'action': [
                'xcopy /efy',
                '$(ConfigurationName)\*.dll',
                '$(OutDir)',
              ],
            },
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
            'VCManifestTool': {
              'AdditionalManifestFiles': [
                'cefsimple/cefsimple.exe.manifest',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-l$(ConfigurationName)/libcef.lib',
            ],
          },
          'library_dirs': [
            # Needed to find cef_sandbox.lib using #pragma comment(lib, ...).
            '$(ConfigurationName)',
          ],
          'sources': [
            '<@(includes_win)',
            '<@(cefsimple_sources_win)',
          ],
        }],
        [ 'OS=="win" and multi_threaded_dll', {
          'configurations': {
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 3,
                  'WarnAsError': 'false',
                },
              },
            },
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 2,
                  'WarnAsError': 'false',
                },
              },
            }
          }
        }],
        [ 'OS=="mac"', {
          'product_name': 'cefsimple',
          'dependencies': [
            'cefsimple_helper_app',
          ],
          'copies': [
            {
              # Add libraries and helper app.
              'destination': '<(PRODUCT_DIR)/cefsimple.app/Contents/Frameworks',
              'files': [
                '$(CONFIGURATION)/libplugin_carbon_interpose.dylib',
                '<(PRODUCT_DIR)/cefsimple Helper.app',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Add framework',
              'action': [
                'cp',
                '-Rf',
                '${CONFIGURATION}/<(framework_name).framework',
                '${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Frameworks/'
              ],
            },
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/<(framework_name)',
                '@executable_path/../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
            {
              # This postbuid step is responsible for creating the following
              # helpers:
              #
              # cefsimple Helper EH.app and cefsimple Helper NP.app are created
              # from cefsimple Helper.app.
              #
              # The EH helper is marked for an executable heap. The NP helper
              # is marked for no PIE (ASLR).
              'postbuild_name': 'Make More Helpers',
              'action': [
                'tools/make_more_helpers.sh',
                'Frameworks',
                'cefsimple',
              ],
            },
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(cefsimple_sources_mac)',
          ],
        }],
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/',
              'files': [
                'Resources/cef.pak',
                'Resources/devtools_resources.pak',
                'Resources/locales/',
                '$(BUILDTYPE)/chrome-sandbox',
                '$(BUILDTYPE)/libcef.so',
                '$(BUILDTYPE)/libffmpegsumo.so',
              ],
            },
          ],
          'dependencies': [
            'gtk',
          ],
          'link_settings': {
            'ldflags': [
              # Look for libcef.so in the current directory. Path can also be
              # specified using the LD_LIBRARY_PATH environment variable.
              '-Wl,-rpath,.',
            ],
            'libraries': [
              "$(BUILDTYPE)/libcef.so",
            ],
          },
          'sources': [
            '<@(includes_linux)',
            '<@(cefsimple_sources_linux)',
          ],
        }],
      ],
    },
    {
      'target_name': 'libcef_dll_wrapper',
      'type': 'static_library',
      'msvs_guid': 'A9D6DC71-C0DC-4549-AEA0-3B15B44E86A9',
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_capi)',
        '<@(includes_wrapper)',
        '<@(libcef_dll_wrapper_sources_common)',
      ],
      'xcode_settings': {
        # Target build path.
        'SYMROOT': 'xcodebuild',
      },
      'conditions': [
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'dependencies': [
            'gtk',
          ],
        }],
        [ 'OS=="win" and multi_threaded_dll', {
          'configurations': {
            'Debug': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 3,
                  'WarnAsError': 'false',
                },
              },
            },
            'Release': {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeLibrary': 2,
                  'WarnAsError': 'false',
                },
              },
            }
          }
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': 'cefclient_helper_app',
          'type': 'executable',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'product_name': 'cefclient Helper',
          'mac_bundle': 1,
          'dependencies': [
            'libcef_dll_wrapper',
          ],
          'defines': [
            'USING_CEF_SHARED',
          ],
          'include_dirs': [
            '.',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
            ],
          },
          'sources': [
            '<@(cefclient_sources_mac_helper)',
          ],
          # TODO(mark): Come up with a fancier way to do this.  It should only
          # be necessary to list helper-Info.plist once, not the three times it
          # is listed here.
          'mac_bundle_resources!': [
            'cefclient/mac/helper-Info.plist',
          ],
          # TODO(mark): For now, don't put any resources into this app.  Its
          # resources directory will be a symbolic link to the browser app's
          # resources directory.
          'mac_bundle_resources/': [
            ['exclude', '.*'],
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'cefclient/mac/helper-Info.plist',
          },
          'postbuilds': [
            {
              # The framework defines its load-time path
              # (DYLIB_INSTALL_NAME_BASE) relative to the main executable
              # (chrome).  A different relative path needs to be used in
              # cefclient_helper_app.
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/<(framework_name)',
                '@executable_path/../../../../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
          ],
        },  # target cefclient_helper_app
        {
          'target_name': 'cefsimple_helper_app',
          'type': 'executable',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'product_name': 'cefsimple Helper',
          'mac_bundle': 1,
          'dependencies': [
            'libcef_dll_wrapper',
          ],
          'defines': [
            'USING_CEF_SHARED',
          ],
          'include_dirs': [
            '.',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/<(framework_name).framework/<(framework_name)',
            ],
          },
          'sources': [
            '<@(cefsimple_sources_mac_helper)',
          ],
          # TODO(mark): Come up with a fancier way to do this.  It should only
          # be necessary to list helper-Info.plist once, not the three times it
          # is listed here.
          'mac_bundle_resources!': [
            'cefsimple/mac/helper-Info.plist',
          ],
          # TODO(mark): For now, don't put any resources into this app.  Its
          # resources directory will be a symbolic link to the browser app's
          # resources directory.
          'mac_bundle_resources/': [
            ['exclude', '.*'],
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'cefsimple/mac/helper-Info.plist',
          },
          'postbuilds': [
            {
              # The framework defines its load-time path
              # (DYLIB_INSTALL_NAME_BASE) relative to the main executable
              # (chrome).  A different relative path needs to be used in
              # cefsimple_helper_app.
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/<(framework_name)',
                '@executable_path/../../../../Frameworks/<(framework_name).framework/<(framework_name)',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
          ],
        },  # target cefsimple_helper_app
      ],
    }],  # OS=="mac"
    [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
      'targets': [
        {
          'target_name': 'gtk',
          'type': 'none',
          'variables': {
            # gtk requires gmodule, but it does not list it as a dependency
            # in some misconfigured systems.
            'gtk_packages': 'gmodule-2.0 gtk+-2.0 gthread-2.0 gtk+-unix-print-2.0',
          },
          'direct_dependent_settings': {
            'cflags': [
              '$(shell <(pkg-config) --cflags <(gtk_packages))',
            ],
          },
          'link_settings': {
            'ldflags': [
              '$(shell <(pkg-config) --libs-only-L --libs-only-other <(gtk_packages))',
            ],
            'libraries': [
              '$(shell <(pkg-config) --libs-only-l <(gtk_packages))',
            ],
          },
        },
        {
          'target_name': 'gtkglext',
          'type': 'none',
          'variables': {
            # gtkglext is required by the cefclient OSR example.
            'gtk_packages': 'gtkglext-1.0',
          },
          'direct_dependent_settings': {
            'cflags': [
              '$(shell <(pkg-config) --cflags <(gtk_packages))',
            ],
          },
          'link_settings': {
            'ldflags': [
              '$(shell <(pkg-config) --libs-only-L --libs-only-other <(gtk_packages))',
            ],
            'libraries': [
              '$(shell <(pkg-config) --libs-only-l <(gtk_packages))',
            ],
          },
        },
      ],
    }],  # OS=="linux" or OS=="freebsd" or OS=="openbsd"
  ],
}
