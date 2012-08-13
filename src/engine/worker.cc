// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Platform-independent worker class implementation. This is subclassed by the
// platform-specific worker classes.

#include "engine/worker.h"

#ifdef __PLATFORM_HAS_BOOST__
#include <boost/uuid/uuid_generators.hpp>
#endif

#include "platforms/common.pb.h"
#include "platforms/unix/messaging_streamsockets.h"

DEFINE_string(platform, "AUTO", "The platform we are running on, or AUTO for "
              "attempting automatic discovery.");
DEFINE_string(coordinator_uri, "", "The URI to contact the coordinator at.");
DEFINE_string(name, "", "A friendly name for this worker.");

namespace firmament {

#ifdef __PLATFORM_HAS_BOOST__
using boost::posix_time::ptime;
using boost::posix_time::second_clock;
using boost::posix_time::seconds;
#endif

Worker::Worker(PlatformID platform_id)
  : platform_id_(platform_id),
    coordinator_uri_(FLAGS_coordinator_uri),
    m_adapter_(new StreamSocketsMessaging()),
    chan_(StreamSocketsChannel<Message>::SS_TCP),
    exit_(false),
    uuid_(GenerateUUID()) {
  string hostname = "";  // platform_.GetHostname();
  VLOG(1) << "Worker starting on host " << hostname << ", platform "
          << platform_id;
  // Start up a worker according to the platform parameter
  switch (platform_id) {
    case PL_UNIX: {
      // Initiate UNIX worker.
      // worker_ = new UnixWorker();
      break;
    }
    default:
      LOG(FATAL) << "Unimplemented!";
  }

  // TODO(malte): fix this!
  resource_desc_.set_uuid(boost::uuids::to_string(uuid_));

  if (!FLAGS_name.empty())
    resource_desc_.set_name(FLAGS_name);

  resource_desc_.set_task_capacity(1);
}

void Worker::AwaitNextMessage() {
  Envelope<Message> envelope(&resource_desc_);
  VLOG_EVERY_N(2, 1) << "Sending (sync)...";
  chan_.SendS(envelope);
  VLOG_EVERY_N(2, 1) << "Waiting for next message...";
  boost::this_thread::sleep(seconds(10));
}

bool Worker::ConnectToCoordinator(const string& coordinator_uri) {
  return m_adapter_->EstablishChannel(coordinator_uri, &chan_);
  // TODO(malte): Send registration message
}

ResourceID_t Worker::GenerateUUID() {
  boost::uuids::random_generator gen;
  return gen();
}

void Worker::Run() {
  // Worker starting -- first need to find a coordinator and connect to it.
  if (coordinator_uri_.empty()) {
    if (!RunCoordinatorDiscovery(coordinator_uri_)) {
      LOG(FATAL) << "No coordinator URI set, and automatic coordinator "
                 << "discovery failed! Exiting...";
    }
  }

  // We now know where the coordinator is. Establish a connection to it.
  CHECK(ConnectToCoordinator(coordinator_uri_))
      << "Failed to connect to the coordinator at " + coordinator_uri_;

  while (!exit_) {  // main loop
    // Wait for events
    AwaitNextMessage();
  }

  // We have dropped out of the main loop and are exiting
  // TODO(malte): any cleanup we need to do; terminate running
  // tasks etc.
  VLOG(1) << "Dropped out of main loop -- cleaning up...";
}

bool Worker::RunCoordinatorDiscovery(const string& coordinator_uri) {
  LOG(FATAL) << "Coordinator auto-discovery is not implemented yet. "
             << "coordinator_uri given was: " << coordinator_uri;
  return false;
}

}  // namespace firmament
