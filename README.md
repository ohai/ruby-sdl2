# Ruby/SDL2

The simple ruby extension library for SDL 2.x.

# Install
Before installing Ruby/SDL2, you need to install:

* [devkit](https://rubyinstaller.org/add-ons/devkit.html) (only for windows)
* M4 (only for installing from git repository)
* [SDL 2.x](https://github.com/libsdl-org/SDL/releases)
* [SDL_image](https://github.com/libsdl-org/SDL_image)
* [SDL_mixer](https://github.com/libsdl-org/SDL_mixer)
* [SDL_ttf](https://github.com/libsdl-org/SDL_ttf)

After installing these libraries, you can install Ruby/SDL2 by gem command as follows:

    gem install ruby-sdl2
    
Alternatively You can also install the master version of Ruby/SDL2 from github:

    git clone https://github.com/ohai/ruby-sdl2.git
    cd ruby-sdl2
    rake gem
    gem install pkg/ruby-sdl2-x.y.z.gem

# Document

* [English Reference manual](https://ohai.github.io/ruby-sdl2/doc-en/)

# License

LGPL v3 (see {file:COPYING.txt}).

The API documentation is based on [SDL2 Wiki](https://wiki.libsdl.org/SDL2/FrontPage), which is distributed under CC-BY 4.0.

# Author

Ippei Obayashi <ohai@kmc.gr.jp>
