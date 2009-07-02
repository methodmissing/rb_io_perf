require "aio"
require "benchmark"

TESTS = 100
INPUT = 'aio.c'.freeze
=begin
Benchmark.bmbm do |results|
  results.report("IO.aio_read") { TESTS.times { IO.aio_read( INPUT ) } }  
  results.report("IO.read") { TESTS.times { IO.read( INPUT ) } }
end
=end

puts IO.aio_read( 'first.txt', 'second.txt', 'third.txt' ).inspect