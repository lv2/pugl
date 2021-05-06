# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

config = configuration_data()
config.set('NAME', 'pugl_clipboard_demo')

info_plist = configure_file(configuration: config,
                            input: files('../../resources/Info.plist.in'),
                            output: 'Info.plist')

subdir('MacOS')
