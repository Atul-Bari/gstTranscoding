#include "gst.h"
#include <gst/audio/audio.h>
#include <gst/app/gstappsrc.h>

#define SAMPLE_RATE 44100
GMainLoop *gstreamer_receive_main_loop = NULL;
void gstreamer_receive_start_mainloop(void) {
  gstreamer_receive_main_loop = g_main_loop_new(NULL, FALSE);

  g_main_loop_run(gstreamer_receive_main_loop);
}

static gboolean gstreamer_receive_bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  switch (GST_MESSAGE_TYPE(msg)) {

  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    exit(1);
    break;

  case GST_MESSAGE_ERROR: {
    gchar *debug;
    GError *error;

    gst_message_parse_error(msg, &error, &debug);
    g_free(debug);

    g_printerr("Error: %s\n", error->message);
    g_error_free(error);
    exit(1);
  }
  default:
    break;
  }

  return TRUE;
}

GstFlowReturn gstreamer_send_new_sample_handler(GstElement *object, gpointer user_data) {
  GstSample *sample = NULL;
  GstBuffer *buffer = NULL;
  gpointer copy = NULL;
  gsize copy_size = 0;
  //SampleHandlerUserData *s = (SampleHandlerUserData *)user_data;
  g_print("\nIn pull sample\n");
  g_signal_emit_by_name (object, "pull-sample", &sample);
  if (sample) {
    buffer = gst_sample_get_buffer(sample);
    if (buffer) {
      gst_buffer_extract_dup(buffer, 0, gst_buffer_get_size(buffer), &copy, &copy_size);
      goHandlePipelineBuffer(copy, copy_size, GST_BUFFER_DURATION(buffer), (GstElement*)user_data);
    }
    gst_sample_unref (sample);
  }

  return GST_FLOW_OK;
}

GstElement *gstreamer_receive_create_pipeline(char *pipeline) {
  gst_init(NULL, NULL);
  GError *error = NULL;
  return gst_parse_launch(pipeline, &error);
}

void gstreamer_receive_start_pipeline(GstElement *pipeline) {
  GstAudioInfo info;
  GstCaps *audio_caps;
  

  g_print("\nIn c start_pipeline ");
  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, gstreamer_receive_bus_call, NULL);
  gst_object_unref(bus);

  GstElement *src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
  audio_caps = gst_audio_info_to_caps (&info);
  g_object_set (src, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);

  g_print("\nIn c  sink start_pipeline ");
  GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "appsink");
  g_object_set(appsink, "emit-signals", TRUE, NULL);
  g_signal_connect(appsink, "new-sample", G_CALLBACK(gstreamer_send_new_sample_handler), pipeline);
  gst_object_unref(appsink);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void gstreamer_receive_stop_pipeline(GstElement *pipeline) { gst_element_set_state(pipeline, GST_STATE_NULL); }

void gstreamer_receive_push_buffer(GstElement *pipeline, void *buffer, int len) {
  GstElement *src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
  GstFlowReturn ret;
  g_print("\nIn c  src gstreamer_receive_push_buffer ");
  if (src != NULL) {
    g_print("\nIn  if gstreamer_receive_push_buffer ");
    gpointer p = g_memdup(buffer, len);
    GstBuffer *buffer = gst_buffer_new_wrapped(p, len);
    ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    if (ret != GST_FLOW_OK) {
    /* We got some error, stop sending data */
    g_print("\nCant Push ");
  }

  g_print("Push success");
    gst_object_unref(src);
  }
}

