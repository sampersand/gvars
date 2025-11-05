# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/extensiontask'
require 'minitest/test_task'

task default: [:compile]

Rake::ExtensionTask.new('gvars') do |ext|
  ext.lib_dir = 'lib/gvars'
end

# Minitest::TestTask.create

# task default: :test

desc 'clean compiled files'
task :clean do
  require 'fileutils'
  FileUtils.rm_rf 'ext/gvars/Makefile'
  FileUtils.rm_rf 'ext/gvars/gvars.so'
  FileUtils.rm_rf 'ext/gvars/gvars.bundle'
  FileUtils.rm_rf 'lib/gvars/gvars.so'
  FileUtils.rm_rf 'lib/gvars/gvars.bundle'
end
