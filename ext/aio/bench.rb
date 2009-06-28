require "aio"
require "benchmark"

TESTS = 100
INPUT = 'aio.c'.freeze
Benchmark.bmbm do |results|
  results.report("IO.aio_read") { TESTS.times { IO.aio_read( INPUT ) } }  
  results.report("IO.read") { TESTS.times { IO.read( INPUT ) } }
end