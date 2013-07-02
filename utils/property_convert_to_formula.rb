#!/usr/bin/ruby
require 'fileutils'
require 'tempfile'

# a quick and dirty tool to take any properties inside an object which are single-line, unquoted properties, and make them quoted so they get evaluated as immutable formulas.


	$in_properties = false
	$in_a_property = false


# Create a temporary file for the modifications.
$temp_file = Tempfile.new('appended')

def search()
	def search_for_property_terminator(the_string)
		if the_string.reverse.index(",\"")
			$in_a_property = false
		elsif the_string.reverse.index("\"")
			$in_properties = false
		elsif the_string.index("}")
			$in_properties = false 
		end
		$temp_file.puts the_string
	end

	def add_quotes_to_line (new_string)
		new_string.gsub!( /(\d)+/ ) do |txt|
			"\"#{txt}\""
		end
		$temp_file.puts new_string
	end

	File.open(ARGV[0], 'r').each_line do |line|
		if line.include? "properties"
			if (not line.index("#")) or (line.index( "properties") < line.index("#"))
				$temp_file.puts line
				$in_properties = true
			else
				$temp_file.puts line
			end
		elsif $in_properties and (not $in_a_property)
			if line.index(":")
				$in_a_property = true
				if $in_a_property and (not line.index("\""))
					add_quotes_to_line(line)
					$in_a_property = false
				end
				if $in_a_property
					#search_for_property_terminator( (line[line.index(":")+1..line.length-1]).index("\"") )
					search_for_property_terminator( line )
				end
			end
			if line.index("}")
				$in_properties = false 
			end
		elsif $in_properties and $in_a_property
			search_for_property_terminator(line)
		elsif
			$temp_file.puts line
		end
	end
end

search()

FileUtils.mv $temp_file.path, ARGV[0]
