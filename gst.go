// Package gst provides an easy API to create an appsrc pipeline
package main

/*
#cgo pkg-config: gstreamer-1.0 gstreamer-app-1.0 gstreamer-audio-1.0

#include "gst.h"

*/
import "C"
import (
	"fmt"
	"sync"
	"unsafe"

	"github.com/pion/webrtc/v2"
	"github.com/pion/webrtc/v2/pkg/media"

)

var pipelinesLock sync.Mutex

// StartMainLoop starts GLib's main loop
// It needs to be called from the process' main thread
// Because many gstreamer plugins require access to the main thread
// See: https://golang.org/pkg/runtime/#LockOSThread
func StartMainLoop() {
	C.gstreamer_receive_start_mainloop()
}

// Pipeline is a wrapper for a GStreamer Pipeline
type Pipeline struct {
	Pipeline *C.GstElement
	tracks    []*webrtc.Track
}

// CreatePipeline creates a GStreamer Pipeline
func CreatePipeline(codecName string, tracks []*webrtc.Track) *Pipeline {
	fmt.Printf("In create pipeline")
	pipelineStr := ""
	switch codecName {
	case "VP8":
		pipelineStr += ", encoding-name=VP8-DRAFT-IETF-01 ! rtpvp8depay ! decodebin ! autovideosink"
	case "Opus":
		pipelineStr += "appsrc name=src ! decodebin ! audioconvert ! audioresample ! audio/x-raw, rate=8000 ! mulawenc ! appsink name=appsink max-buffers=1"
	// case webrtc.VP9:
	// 	pipelineStr += " ! rtpvp9depay ! decodebin ! autovideosink"
	// case webrtc.H264:
	// 	pipelineStr += " ! rtph264depay ! decodebin ! autovideosink"
	// case webrtc.G722:
	// 	pipelineStr += " clock-rate=8000 ! rtpg722depay ! decodebin ! autoaudiosink"
	default:
		panic("Unhandled codec " + codecName)
	}

	pipelineStrUnsafe := C.CString(pipelineStr)
	defer C.free(unsafe.Pointer(pipelineStrUnsafe))
	return &Pipeline{
		Pipeline: C.gstreamer_receive_create_pipeline(pipelineStrUnsafe),
		tracks: tracks,
	}
}

// Start starts the GStreamer Pipeline
func (p *Pipeline) Start() {
	fmt.Printf("\nIn start")
	C.gstreamer_receive_start_pipeline(p.Pipeline)
}

// Stop stops the GStreamer Pipeline
func (p *Pipeline) Stop() {
	C.gstreamer_receive_stop_pipeline(p.Pipeline)
}

//export goHandlePipelineBuffer
func goHandlePipelineBuffer(buffer unsafe.Pointer, bufferLen C.int, duration C.int, pipe *C.GstElement) {
	fmt.Println("Pulled...")
	//pipelinesLock.Lock()
	fmt.Println("Pulled...")
	// pipeline, ok := pipelines[int(pipelineID)]
	//pipelinesLock.Unlock()

	// if ok {
	 	samples := uint32(8000 * (float32(duration) / 1000000000))
	 	for _, t := range pgst.tracks {
	 		if err := t.WriteSample(media.Sample{Data: C.GoBytes(buffer, bufferLen), Samples: samples}); err != nil {
	 			panic("Error with tracks")
	 		}
	 	}
	// } else {
	// 	fmt.Printf("discarding buffer, no pipeline with id")
	// }
	C.free(buffer)
}

// Push pushes a buffer on the appsrc of the GStreamer Pipeline
func (p *Pipeline) Push(buffer []byte) {
	fmt.Printf("In Push")
	b := C.CBytes(buffer)
	defer C.free(b)
	C.gstreamer_receive_push_buffer(p.Pipeline, b, C.int(len(buffer)))
}

/*func main() {
	fmt.Printf("In main")
	p := CreatePipeline("Opus")
	p.Start()
	data := make([]byte, 320*240*3)
	fmt.Println(data)
	p.Push(data)
	p.Stop()

}*/


