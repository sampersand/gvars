# frozen_string_literal: true

require_relative "gvars/version"
require_relative "gvars/gvars"

module GVars
=begin
  # TODO: make this non-eval; unfortunately, it's not easy to fully emulate right now. maybe in the future?
  #
  # NOTE: THIS DOESN'T SUPPORT `$_` or regex vars!
  module_function def global_variable_defined?(key)
    warn "GVArs.global_variable_defined? is currently experimental", category: :experimental, uplevel: 1
    key = Symbol === key ? key : key.to_str.to_sym

    unless key.match? /\A\$[[:alpha:]_][[:alnum:]_]*\z/
      raise NameError, format("'%s' is not allowed as a global variable name", key)
    end

    !! eval "defined? #{key}"
  end
=end
end
