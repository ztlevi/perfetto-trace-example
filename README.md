``` sh
crr
/usr/bin/time -al ./release/data_generator_test
/usr/bin/time -al  ~/dev-remote/perfetto/out/release/trace_processor_shell --httpd --full-sort example.pftrace

traceconv text example.pftrace > example.textproto
```

Then open the example.pftrace with perfetto
