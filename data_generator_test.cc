// #include "perfetto_trace.pb.h"
#include "trace_categories.h"
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
  cfg.add_buffers()->set_size_kb(1024 * 1024);
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

  protozero::HeapBuffered<perfetto::protos::pbzero::Trace> additional_trace;
  auto last_packet = additional_trace->add_packet();
  last_packet->set_trusted_uid(505);
  last_packet->set_timestamp(trace_end_time);
  last_packet->set_trusted_packet_sequence_id(1);
  auto *last_packet_service_event = last_packet->set_service_event();
  last_packet_service_event->set_tracing_disabled(true);
  std::vector<unsigned char> additional_trace_u_chars =
      additional_trace.SerializeAsArray();
  std::vector<char> additional_trace_chars(
      additional_trace_u_chars.size());
  std::transform(additional_trace_u_chars.begin(),
                 additional_trace_u_chars.end(),
                 additional_trace_chars.begin(),
                 [](unsigned char c) { return static_cast<char>(c); });

  std::ofstream output;
  output.open("example.pftrace", std::ios::out | std::ios::binary);
  output.write(&raw_trace[0], raw_trace.size());
  output.write(&additional_trace_chars[0], additional_trace_chars.size());
  output.close();
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
