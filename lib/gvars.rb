# frozen_string_literal: true

require_relative "gvars/version"
require_relative "gvars/gvars"

module GVars
  class << self
    alias get  global_variable_get
    alias []  global_variable_get
    alias set  global_variable_set
    alias []= global_variable_set
    alias alias alias_global_variable
    alias list  global_variables
  end
end
