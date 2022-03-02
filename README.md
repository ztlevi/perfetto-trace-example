``` sh
cbb
./build/data_generator_test
trace_processor --httpd --full-sort example.pftrace
traceconv text example.pftrace > example.textproto
```

Then open the example.pftrace with perfetto
