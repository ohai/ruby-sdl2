# frozen_string_literal: true

source "https://rubygems.org"

group :development do
  gem "rake"
  gem "yard"
  gem "sord", path: "../sord/main"  # For locally modified sord, PR is already sent.
  gem "steep", "~> 1.3.0"  # sord depends on rbs 2.x, but steep 1.4.x depends rbs 3.x
  gem "parallel", "~> 1.22.0"  #  parallel >= 1.23.0 breaks compatibility
end
