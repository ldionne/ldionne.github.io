<%
require 'open3'
require 'pathname'
require 'tilt'


def rvo?(n, file)
    template = Pathname.new(file).expand_path
    cpp = Pathname.new('__tmp.cpp').expand_path
    exe = Pathname.new('__tmp').expand_path
    command = "clang++ -O3 -ftemplate-depth=-1 -std=c++14 -o #{exe} #{cpp}"

    code = Tilt::ERBTemplate.new(template).render(nil, input_size: n)
    cpp.write(code)
    stdout, stderr, status = Open3.capture3(command)
    raise "compilation error: #{stderr}\n\n#{code}" if not status.success?

    if `#{exe}` =~ /no-rvo/
        return "no"
    else
        return "yes"
    end

ensure
    cpp.delete if cpp.exist?
    exe.delete if exe.exist?
end

RANGE = [10, 20, 30, 40, 50, 100, 200, 300]

%>

|          n           | <%= RANGE.join(' | ') %>                                         |
| -------------------- | <%= RANGE.map { ':-:' }.join(' | ') %>                           |
| recursive atoms      | <%= RANGE.map { |n| rvo?(n, 'atoms.erb.cpp') }.join(' | ') %>    |
| flat                 | <%= RANGE.map { |n| rvo?(n, 'flat.erb.cpp') }.join(' | ') %>     |
| baseline             | <%= RANGE.map { |n| rvo?(n, 'baseline.erb.cpp') }.join(' | ') %> |
