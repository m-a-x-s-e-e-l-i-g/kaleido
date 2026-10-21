MODULE_NAME = kaleido
MODULE_OBJS = $(BUILD_DIR)/kaleido.o $(BUILD_DIR)/kaleido_driver.o $(BUILD_DIR)/kaleido_settings.o $(BUILD_DIR)/kaleido_raw_trace.o $(BUILD_DIR)/kaleido_capture_gate.o $(BUILD_DIR)/fx_camera.o $(BUILD_DIR)/fx_mirror.o $(BUILD_DIR)/fx_mirror_raw.o $(BUILD_DIR)/fx_mirror_sensor.o $(BUILD_DIR)/fx_pixels.o $(BUILD_DIR)/gl_format.o $(BUILD_DIR)/gl_review.o
MODULE_CFLAGS += -Ishared -Iengine -include shared/fx_namespace.h -DFX_PREFIX=kaleido -DMIRROR_RELEASE
TOP_DIR = ../..
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete
$(BUILD_DIR)/fx_camera.o: shared/fx_camera.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<
$(BUILD_DIR)/fx_pixels.o: shared/fx_pixels.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(CDEPS_FLAGS) -c -o $@ $<
$(BUILD_DIR)/gl_format.o: engine/gl_format.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<
$(BUILD_DIR)/gl_review.o: engine/gl_review.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(CDEPS_FLAGS) -c -o $@ $<

$(BUILD_DIR)/fx_mirror.o: shared/fx_mirror.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(CDEPS_FLAGS) -c -o $@ $<
$(BUILD_DIR)/fx_mirror_raw.o: shared/fx_mirror_raw.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(CDEPS_FLAGS) -c -o $@ $<
$(BUILD_DIR)/fx_mirror_sensor.o: shared/fx_mirror_sensor.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(CDEPS_FLAGS) -c -o $@ $<
