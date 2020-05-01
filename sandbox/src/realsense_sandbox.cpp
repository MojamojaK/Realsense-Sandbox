#include <cstdlib>
#include <vector>
#include <iostream>

#include <librealsense2/rs.hpp>

#define LOOP_COUNT 10
#define FRAMES_TO_CAPTURE 4

typedef struct stream_profile_config
{
	int fps;
	rs2_stream stream_type;
	int stream_index;
	rs2_format format;
	int width;
	int height;
	
} stream_profile_config;

std::tuple<bool, rs2::stream_profile> get_stream_profile(const rs2::sensor &sensor, const stream_profile_config &config)
{
	const std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
	for (const rs2::stream_profile& stream_profile : stream_profiles)
	{
		if (stream_profile.is<rs2::video_stream_profile>())
		{
			const rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();
			if (video_stream_profile.fps() == config.fps 
				&& video_stream_profile.stream_type() == config.stream_type 
				&& video_stream_profile.stream_index() == config.stream_index
				&& video_stream_profile.format() == config.format
				&& video_stream_profile.width() == config.width
				&& video_stream_profile.height() == config.height)
			{
				return std::make_tuple(true, stream_profile);
			}
		}
	};
	return std::make_tuple(false, stream_profiles[0]);
}

int main(void)
{
	std::cout << "[INFO] Staring Realsense Sandbox Program. Librealsense Version: " << RS2_API_VERSION_STR << std::endl;
	rs2::context context;

	const rs2::device_list device_list = context.query_devices();
	if (device_list.size() < 1)
	{
		std::cout << "[ERROR] NO REALSENSE DEVICES FOUND!" << std::endl;
		return EXIT_FAILURE;
	}

	const rs2::device device = device_list[0];
	if (!device)
	{
		std::cout << "[ERROR] REALSENSE DEVICE INVALID" << std::endl;
		return EXIT_FAILURE;
	}

	const rs2::color_sensor color_sensor = device.first<rs2::color_sensor>();
	const rs2::depth_sensor depth_sensor = device.first<rs2::depth_sensor>();
	if (!color_sensor || !depth_sensor)
	{
		std::cout << "[ERROR] REALSENSE DEVICE SENSOR(S) INVALID" << std::endl;
		return EXIT_FAILURE;
	}

	const std::tuple<bool, rs2::stream_profile> color_stream_profile_result = get_stream_profile(color_sensor, { 6, rs2_stream::RS2_STREAM_COLOR, 0, rs2_format::RS2_FORMAT_BGR8, 1280, 720 });
	const std::tuple<bool, rs2::stream_profile> depth_stream_profile_result = get_stream_profile(depth_sensor, { 6, rs2_stream::RS2_STREAM_DEPTH, 0, rs2_format::RS2_FORMAT_Z16, 1280, 720 });
	if (!std::get<0>(color_stream_profile_result) || !std::get<0>(depth_stream_profile_result))
	{
		std::cout << "[ERROR] REALSENSE SENSOR STREAM PROFILE NOT FOUND" << std::endl;
		return EXIT_FAILURE;
	}

	const rs2::stream_profile &color_stream_profile = std::get<1>(color_stream_profile_result);
	const rs2::stream_profile &depth_stream_profile = std::get<1>(depth_stream_profile_result);
	
	for (int loop = 0; loop < LOOP_COUNT; loop++)
	{
		std::cout << "[INFO] LOOP " << loop << std::endl;
		{
			rs2::syncer syncer;
			std::cout << "[INFO] rs2::syncer constructed" << std::endl;

			std::cout << "[INFO] Opening / Starting Sensors" << std::endl;
			color_sensor.open(color_stream_profile);
			depth_sensor.open(depth_stream_profile);
			color_sensor.start(syncer);
			depth_sensor.start(syncer);
			std::cout << "[INFO] Opening / Starting Sensors Complete" << std::endl;

			for (int frame_number = 0; frame_number < FRAMES_TO_CAPTURE; frame_number++)
			{
				rs2::frameset tmp_frameset;
				while (syncer.poll_for_frames(&tmp_frameset)) {}
				const rs2::frameset frameset = syncer.wait_for_frames(8000);
			}

			std::cout << "[INFO] Stopping / Closing Sensors" << std::endl;
			color_sensor.stop();
			depth_sensor.stop();
			color_sensor.close();
			depth_sensor.close();
			std::cout << "[INFO] Stopping / Closing Sensors Complete" << std::endl;
		}
		std::cout << "[INFO] rs2::syncer destructed" << std::endl;
		std::cout << "[INFO] Input Something to continue..." << std::endl;
		std::string input;
		std::cin >> input;
	}
	
	return EXIT_SUCCESS;
}