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

std::shared_ptr<perfetto::TracingSession>
StartTracing(std::string output_filename) {
  // The trace config defines which types of data sources are enabled for
  // recording. In this example we just need the "track_event" data source,
  // which corresponds to the TRACE_EVENT trace points.
  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(32 * 1024);
  auto *ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");

  cfg.set_write_into_file(true);
  cfg.set_file_write_period_ms((uint32_t)1000);
  cfg.set_flush_period_ms((uint32_t)1000);
  std::remove(&output_filename[0]);
  cfg.set_output_path(output_filename);

  auto tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
  std::shared_ptr<perfetto::TracingSession> tracing_session_shared =
      std::move(tracing_session);
  return tracing_session_shared;
}

void StopTracing(std::shared_ptr<perfetto::TracingSession> tracing_session,
                 std::string output_filename, uint64_t trace_end_time) {
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
  std::vector<char> additional_trace_chars(additional_trace_u_chars.size());
  std::transform(additional_trace_u_chars.begin(),
                 additional_trace_u_chars.end(), additional_trace_chars.begin(),
                 [](unsigned char c) { return static_cast<char>(c); });

  // Append the additional packet
  std::fstream file;
  file.open(output_filename, std::ios_base::app);
  if (file.is_open())
    file.write(additional_trace_chars.data(), additional_trace_chars.size());
}

void create_counter_track(
    std::shared_ptr<perfetto::TracingSession> tracing_session,
    std::vector<float> data, uint64_t start_time, std::string track_name,
    std::string unit_name, uint64_t bin_width) {
  uint64_t increment_time = start_time;
  perfetto::CounterTrack counter_track =
      perfetto::CounterTrack(&track_name[0], &unit_name[0]);

  int flush_count = 100;
  int flush_counter = 0;
  uint64_t flush_timeout = 5000;

  for (auto val : data) {
    TRACE_COUNTER("op1", counter_track, increment_time, val);
    increment_time += bin_width;
    if (flush_counter++ % flush_count == 0) {
      tracing_session->FlushBlocking(flush_timeout);
    }
  }
  // Add a point in the end to show the counter track's last value
  TRACE_COUNTER("op1", counter_track, increment_time, 0);
}

int main(int, const char **) {
  InitializePerfetto();

  std::string output_filename = "example.pftrace";
  auto tracing_session = StartTracing(output_filename);

  uint32_t time_unit = 1000000000;
  auto start_time = perfetto::TrackEvent::GetTraceTimeNs() + time_unit * 10;
  uint32_t trace_duration = 300 * time_unit; // 300s
  uint64_t trace_end_time = start_time + trace_duration;

  std::vector<float> data;
  int num_of_data = 3000000;
  for (int i = 0; i < num_of_data; ++i) {
    data.push_back((float)(1 + sin((8 * i * M_PI) / num_of_data)));
  }
  uint64_t bin_width = trace_duration / num_of_data;

  std::thread thread([&] {
    create_counter_track(tracing_session, data, start_time, "track1", "unit1",
                         bin_width);
  });
  std::thread thread2([&] {
    create_counter_track(tracing_session, data, start_time, "track2", "unit2",
                         bin_width);
  });
  thread.join();
  thread2.join();

  StopTracing(tracing_session, output_filename, trace_end_time);

  return 0;
}
