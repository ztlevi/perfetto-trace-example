#include "perfetto_trace.pb.h"
#include "trace_categories.h"
#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <thread>
#include <vector>

void InitializePerfetto() {
  perfetto::TracingInitArgs args;
  // The backends determine where trace events are recorded. For this example
  // we are going to use the in-process tracing service, which only includes
  // in-app events.
  args.backends = perfetto::kInProcessBackend;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
}

std::unique_ptr<perfetto::TracingSession> StartTracing() {
  // The trace config defines which types of data sources are enabled for
  // recording. In this example we just need the "track_event" data source,
  // which corresponds to the TRACE_EVENT trace points.
  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(1024*1024);
  auto *ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");

  auto tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
  return tracing_session;
}

void StopTracing(std::unique_ptr<perfetto::TracingSession> tracing_session,
                 uint64_t trace_end_time) {
  perfetto::TrackEvent::Flush();
  tracing_session->StopBlocking();
  std::vector<char> raw_trace = tracing_session->ReadTraceBlocking();
  std::string raw_trace_string(raw_trace.begin(), raw_trace.end());

  // Change the end time of the trace
  perfetto::protos::Trace trace;
  trace.ParseFromString(raw_trace_string);
  auto packet = trace.mutable_packet(7);
  packet->set_timestamp(trace_end_time);
  assert(packet->service_event().tracing_disabled() == true);

  std::fstream output("example.pftrace", std::ios::out | std::ios::binary);
  trace.SerializeToOstream(&output);
}

int main(int, const char **) {
  InitializePerfetto();

  auto tracing_session = StartTracing();

  auto start_time = perfetto::TrackEvent::GetTraceTimeNs();

  uint32_t time_unit = 1000000000;
  uint32_t trace_duration = 3 * time_unit; // 3s
  uint64_t trace_end_time = start_time + trace_duration;

  std::vector<float> data;
  int num_of_data = 30000;
  for (int i = 0; i < num_of_data; ++i) {
    data.push_back((float)(1 + sin((8 * i * M_PI) / num_of_data)));
  }
  perfetto::CounterTrack counter_track =
      perfetto::CounterTrack("track1", "unit1");
  uint64_t increment_time = start_time;
  for (auto val : data) {
    TRACE_COUNTER("op1", counter_track, increment_time, val);
    increment_time += trace_duration / num_of_data;
  }
  // Add a point in the end to show the counter track's last value
  TRACE_COUNTER("op1", counter_track, increment_time, 0);

  StopTracing(std::move(tracing_session), trace_end_time);

  return 0;
}
