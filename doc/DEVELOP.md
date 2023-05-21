# For Developer


## Check out and build a gem

    git clone https://github.com/ohai/ruby-sdl2.git
    cd ruby-sdl2
    bundle install
    bundle exec rake gem  # To build a gem
    bundle exec rake doc  # To build API documents
    
## Build C extension and run samples 

    bundle exec rake build  # To build sdl2_ext.so
    cd sample
    ruby -I .. -I ../lib testspriteminimal.rb

## For type checking

Preparation

    bundle exec rbs collection install
    
Generate ruby-sdl2's rbs file

    bundle exec rake sdl2.rbs

Signature file validation

    bundle exec steep validate

Type checking with Steep

    bundle exec steep check

