function replace_in_file (filename, from, to)
	local text
	local infile
	local outfile
	local oldtext
	local newtext
	infile = io.open(filename, "rb")
	text = infile:read("*all")
	infile:close()
	oldtext = text
	newtext = string.gsub(oldtext, from, to)
	text = newtext
	if newtext == oldtext then
   print("Failed to postprocess '" .. filename .. "': " .. from .. " -> " .. to)
   os.exit(1)
	end
	outfile = io.open(filename, "wb")
	outfile:write(text)
	outfile:close()
end

newaction {
	trigger     = "fixup",
	description = "Fixup Premake",
	execute     = function ()
		replace_in_file("premake5.lua", '"LinkTimeOptimization"', '')
	end
}
