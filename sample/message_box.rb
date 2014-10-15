# -*- coding: utf-8 -*-
require 'sdl2'

SDL2.show_simple_message_box(SDL2::MESSAGEBOX_WARNING, "warning! warning!",
                            "A huge battleship is approaching fast", nil)

button = SDL2.show_message_box(flags: SDL2::MESSAGEBOX_WARNING,
                               window: nil,
                               title: "Enemy is approaching",
                               message: "That was when it all began",
                               buttons: [ { # flags is ignored
                                           id: 0, 
                                           text: "いいえ",
                                          }, 
                                         {flags: SDL2::MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
                                          id: 1,
                                          text: "はい",
                                         },
                                         {flags: SDL2::MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
                                          id: 2,
                                          text: "キャンセル",
                                         },
                                        ],
                               color_scheme: {
                                              bg: [255, 0, 0],
                                              text: [0, 255, 0],
                                              button_border: [255, 0, 0],
                                              button_bg: [0, 0, 255],
                                              button_selected: [255, 0, 0]
                                             }
                              )
p button
                               
