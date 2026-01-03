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

  class << self
    alias get global_variable_get
    alias []  global_variable_get
    alias set global_variable_set
    alias []= global_variable_set
    alias alias alias_global_variable
    alias list global_variables
    # alias defined? global_variable_defined?

    alias to_a list

    include Enumerable

    def each
      return to_enum __method__ unless block_given?

      global_variables.each do |gvar|
        yield gvar, global_variable_get(gvar)
      end
    end

    def to_h = each.to_h
  end
end
