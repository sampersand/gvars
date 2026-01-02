# frozen_string_literal: true

require_relative "lib/gvars/version"

Gem::Specification.new do |spec|
  spec.name = "gvars"
  spec.version = GVars::VERSION
  spec.authors = ["Sam Westerman"]
  spec.email = ["mail@sampersand.me"]

  spec.summary = "Provides utility methods for interacting with global variables"
  spec.description = <<~EOS
    Provides methods for manupulating global variables, as well as defining custom "hooked"
    variables
  EOS
  spec.homepage = "https://github.com/sampersand/gvars"
  spec.license = "MIT"
  spec.required_ruby_version = '>= 3.1.0'

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  gemspec = File.basename(__FILE__)
  spec.files = IO.popen(%w[git ls-files -z], chdir: __dir__, err: IO::NULL) do |ls|
    ls.readlines("\x0", chomp: true).reject do |f|
      (f == gemspec) ||
        f.start_with?(*%w[bin/ test/ spec/ features/ .git appveyor Gemfile])
    end
  end

  spec.require_paths = ["lib"]
  spec.extensions = ["ext/gvars/extconf.rb"]

  # Uncomment to register a new dependency of your gem
  # spec.add_dependency "example-gem", "~> 1.0"
end
