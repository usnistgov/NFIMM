## Configure

See these Doxyfile (file) parameters:

```
PROJECT_NAME          = NFIMM
HTML_EXTRA_STYLESHEET = proj.css
INPUT                 = ../../nfimm  (on or near line 816)
OUTPUT_DIRECTORY      = ../../nfimm/doc
```


## Run

### Windows
The Doxyfile is configured to write output to `.\doc\html` dir.  It references the css file:  `.\doc\proj.css`.

```
.\NFIMM\doc> "C:\Program Files\doxygen\bin\doxygen.exe"
```

### Linux
The Doxyfile is configured to write output to `./doc/html` dir.  It references the css file:  `./doc/prpj.css`.

```
./NFIMM/doc$ doxygen
```
