#!/usr/bin/env python3
# OpenMPT help file scraper
# by coda and Saga Musix
# This script downloads the OpenMPT manual TOC and then downloads all pages
# from that TOC. The pages are parsed and all required image files are fetched.
# The script also generates the appropriate files that can be fed into the
# HTML Help Workshop to generate a CHM file.

from urllib.request import urlopen, urlretrieve
import re, os, shutil, subprocess

base_url = 'http://wiki.openmpt.org'

os.chdir(os.path.dirname(os.path.abspath(__file__)))

shutil.rmtree('html', ignore_errors=True)
shutil.copytree('source', 'html')

style = urlopen(base_url + '/load.php?debug=false&lang=en&modules=mediawiki.legacy.common%2Cshared|mediawiki.ui.button|skins.vector.styles&only=styles&skin=vector&*').read().decode('UTF-8')
# Remove a few unused CSS classes
style = re.sub(r'\}(\w+)?[\.#]vector([\w >]+)\{.+?\}', '}', style)
style_file = open('html/style.css', 'w')
style_file.write(style)
style_file.close()

toc_page = urlopen(base_url + '/index.php?title=Manual:_CHM_TOC&action=render').read().decode('UTF-8')

pages = re.findall('href="' + base_url + '/(.+?)"', toc_page)

def destname(p):
    p = p.split(':_')[1]
    p = p.replace('/', '_')
    p = p.replace('.', '_')
    while p.find('__') >= 0:
        p = p.replace('__', '_')
    if p.find('#') >= 0:
        parts = p.split('#')
        return parts[0] + '.html#' + parts[1]
    return p + '.html'
    
def title(p):
    p = p.split(':_')[1]
    p = p.replace('_', ' ')
    return p

def localurl(p):
    p = destname(p)
    return p

def replace_images(m):
    global base_url
    filepath = m.group(1) + '/' + m.group(2) + '/'
    filename = m.group(3)
    project.write(filename + "\n")

    urlretrieve(base_url + '/images/' + filepath + filename, 'html/' + filename)
    return '"' + filename + '"'

def fix_internal_links(m):
    return '<a href="' + localurl(m.group(1)) + '"'

project = open('html/OpenMPT Manual.hhp', 'w')

project.write("""[OPTIONS]
Compatibility=1.1 or later
Compiled file=OpenMPT Manual.chm
Contents file=OpenMPT Manual.hhc
Display compile progress=No
Full-text search=Yes
Language=0x409 English (United States)
Title=OpenMPT Manual
Default Window=OpenMPT
Default topic=""" + localurl(pages[0]) + """

[WINDOWS]
OpenMPT=,"OpenMPT Manual.hhc",,""" + localurl(pages[0]) + """,,,,,,0x42520,215,0x300e,[20,20,780,580],0xb0000,,,,,,0

[FILES]
style.css
help.css
bullet.png
external.png
""")

for p in pages:
    content = urlopen(base_url + '/index.php?title=' + p + '&action=render').read().decode('UTF-8')
    # Download and replace image URLs
    content = re.sub(r' srcset=".+?"', '', content);
    content = re.sub(r'"/images/thumb/(\w+)/(\w+)/([^\/]+?)/([^\/]+?)"', replace_images, content)
    content = re.sub(r'"/images/(\w+)/(\w+)/([^\/]+?)"', replace_images, content)
    # Remove comments
    content = re.sub(r'<!--(.+?)-->', '', content, flags = re.DOTALL)
    # Fix local URLs
    content = re.sub(r'<a href="' + base_url + '/File:', '<a href="', content)
    content = re.sub(r'<a href="' + base_url + '/(Manual:.+?)"', fix_internal_links, content)
    content = re.sub(r'<a href="/(Manual:.+?)"', fix_internal_links, content)
    # Remove templates that shouldn't turn up in the manual
    content = re.sub(r'<div class="todo".+?</div>', '', content, flags = re.DOTALL);
    content = re.sub(r'<p class="newversion".+?</p>', '', content, flags = re.DOTALL);
    
    section = re.match(r'(.+)/', title(p))
    section_str = ''
    if section:
        section_str =  section.group(1)
    
    content = """<!DOCTYPE html>
    <html lang="en">
    <head>
    <link href="style.css" rel="stylesheet">
    <link href="help.css" rel="stylesheet">
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <title>OpenMPT Manual - """ + title(p) + """</title>
    </head>
    <body>
    <h1>""" + title(p) + '</h1><div id="content" class="mw-body">' + content + '</div></body></html>'
    
    saved = open('html/' + destname(p), 'wb')
    
    saved.write(bytes(content, 'UTF-8'))
    saved.close()
    
    project.write(destname(p)+"\n")
    print(p)
    
project.close()

# Create TOC
toc = open('html/OpenMPT Manual.hhc', 'w')

toc.write("""
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
    <param name="ImageType" value="Folder">
</OBJECT>
""")

def toc_parse(m):
    return """<OBJECT type="text/sitemap">
    <param name="Name" value=\"""" + m.group(2) + """">
    <param name="Local" value=\"""" + localurl(m.group(1)) + """">
    </OBJECT>"""

def toc_parse_chapter(m):
    return """<li><OBJECT type="text/sitemap">
    <param name="Name" value=\"""" + m.group(1) + """">
    </OBJECT>"""

toc_text = re.sub(r'<!--(.+?)-->', '', toc_page, flags = re.DOTALL)
toc_text = re.sub(r'<div(.+?)/div>', '', toc_text, flags = re.DOTALL)
toc_text = re.sub(r'<a href="' + base_url + '/(.+?)".*?>(.+?)</a>', toc_parse, toc_text)
toc_text = re.sub(r'<li> ([^<]+)$', toc_parse_chapter, toc_text, flags = re.MULTILINE)
toc.write(toc_text)

toc.write("""
</BODY></HTML>
""")
toc.close()

if(subprocess.call(['htmlhelp/hhc.exe', '"html/OpenMPT Manual.hhp"']) != 1):
    raise Exception("Something went wrong during manual creation!")
try:
    os.remove('../../packageTemplate/html/OpenMPT Manual.chm')
except OSError:
    pass
shutil.copy2('html/OpenMPT Manual.chm', '../../packageTemplate/')
