#include <cstdlib>
#include <vector>
#include <iostream>

#include <librealsense2/rs.hpp>
#include <future>
#include <map>

constexpr int loop_count = 10;
constexpr size_t frame_capture_count = 4;

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
	std::cout << "[INFO] Starting Realsense Sandbox Program. " << std::endl;
	std::cout << "[INFO] Librealsense Library Version: " << RS2_API_VERSION_STR << std::endl;

	{
		const rs2::context context;

		// Loop LOOP_COUNT times
		for (int i = 0; i < loop_count; i++)
		{
			std::cout << "[INFO] LOOP " << i << std::endl;
			// Get Device List from context
			const rs2::device_list device_list = context.query_devices();
			const uint32_t device_count = device_list.size();
			if (device_count < 1)
			{
				std::cout << "[ERROR] NO REALSENSE DEVICES FOUND!" << std::endl;
				return EXIT_FAILURE;
			}

			std::cout << "[INFO] " << device_count << " REALSENSE DEVICES FOUND!" << std::endl;

			struct result
			{
				uint32_t index;
				int status;
			};

			std::mutex cout_mutex; // Mutex for std::cout race prevention

			std::mutex shared_map_mutex; // Mutex for sensor_map write race prevention
			std::map <uint32_t, rs2::sensor> color_sensor_map;
			std::map <uint32_t, rs2::sensor> depth_sensor_map;
			std::map<uint32_t, rs2::syncer> syncer_map;
			{
				std::cout << "[INFO] REALSENSE DEVICES OPENING" << std::endl;
				const std::chrono::steady_clock::time_point open_start = std::chrono::steady_clock::now();

				std::vector<std::future<struct result>> open_futures;
				open_futures.reserve(device_count);
				for (uint32_t j = 0; j < device_count; j++)
				{
					open_futures.emplace_back(std::async(std::launch::async, [&device_list, &cout_mutex, &shared_map_mutex, &color_sensor_map, &depth_sensor_map, &syncer_map](const uint32_t j) -> struct result
					{
						try {
							const rs2::device device = device_list[j];
							if (!device)
							{
								std::lock_guard<std::mutex> _(cout_mutex);
								std::cout << "[ERROR] REALSENSE DEVICE " << j << " INVALID" << std::endl;
								return { j, EXIT_FAILURE };
							}

							const rs2::color_sensor color_sensor = device.first<rs2::color_sensor>();
							const rs2::depth_sensor depth_sensor = device.first<rs2::depth_sensor>();
							if (!depth_sensor || !color_sensor)
							{
								std::lock_guard<std::mutex> _(cout_mutex);
								std::cout << "[ERROR] REALSENSE DEVICE " << j << " SENSOR(S) INVALID" << std::endl;
								return { j, EXIT_FAILURE };
							}

							const std::tuple<bool, rs2::stream_profile> color_stream_profile_result = get_stream_profile(color_sensor, { 6, rs2_stream::RS2_STREAM_COLOR, 0, rs2_format::RS2_FORMAT_BGR8, 1280, 720 });
							const std::tuple<bool, rs2::stream_profile> depth_stream_profile_result = get_stream_profile(depth_sensor, { 6, rs2_stream::RS2_STREAM_DEPTH, 0, rs2_format::RS2_FORMAT_Z16, 1280, 720 });
							if (!std::get<0>(depth_stream_profile_result) || !std::get<0>(color_stream_profile_result))
							{
								std::lock_guard<std::mutex> _(cout_mutex);
								std::cout << "[ERROR] REALSENSE DEVICE " << j << " SENSOR STREAM PROFILE NOT FOUND" << std::endl;
								return { j, EXIT_FAILURE };
							}

							const rs2::stream_profile &color_stream_profile = std::get<1>(color_stream_profile_result);
							const rs2::stream_profile &depth_stream_profile = std::get<1>(depth_stream_profile_result);

							rs2::syncer syncer;

							{
								std::lock_guard<std::mutex> _(shared_map_mutex);
								color_sensor_map.insert_or_assign(j, color_sensor);
								depth_sensor_map.insert_or_assign(j, depth_sensor);
								syncer_map.insert_or_assign(j, syncer);
							}

							const std::future<void> color_open = std::async(std::launch::async, [&color_sensor, &color_stream_profile, &syncer]()
							{
								color_sensor.open(color_stream_profile);
								color_sensor.start(syncer);
							});

							const std::future<void> depth_open = std::async(std::launch::async, [&depth_sensor, &depth_stream_profile, &syncer]()
							{
								depth_sensor.open(depth_stream_profile);
								depth_sensor.start(syncer);
							});

							color_open.wait();
							depth_open.wait();
							
							std::this_thread::sleep_for(std::chrono::seconds(5));

							return { j, EXIT_SUCCESS };
						}
						catch (const std::exception &e)
						{
							std::lock_guard<std::mutex> _(cout_mutex);
							std::cout << "[ERROR] REALSENSE DEVICE " << j << " THREW ERROR: " << e.what() << std::endl;
							return { j, EXIT_FAILURE };
						}
					}, j));
				}

				// Verify All Color Sensor Opening
				int status = EXIT_SUCCESS;
				std::vector<struct result> open_results;
				open_results.reserve(device_count);
				for (std::future<struct result> &open_future : open_futures)
				{
					const struct result open_result = open_future.get();
					status |= open_result.status;
					open_results.emplace_back(open_result);
				}
				// If at least one sensor failed to open, close all opened sensors and return EXIT_FAILURE
				if (status != EXIT_SUCCESS)
				{
					std::cout << "[INFO] REALSENSE DEVICES OPENING FAILED. EXITING." << std::endl;
					for (const struct result &open_result : open_results)
					{
						if (open_result.status == EXIT_SUCCESS)
						{
							const rs2::sensor &color_sensor = color_sensor_map[open_result.index];
							const rs2::sensor &depth_sensor = depth_sensor_map[open_result.index];
							color_sensor.stop();
							color_sensor.close();
							depth_sensor.stop();
							depth_sensor.stop();
						}
					}
					return EXIT_FAILURE;
				}
				const std::chrono::steady_clock::time_point open_stop = std::chrono::steady_clock::now();
				const time_t open_time = std::chrono::duration_cast<std::chrono::milliseconds>(open_stop - open_start).count();
				
				std::cout << "[INFO] REALSENSE DEVICES OPENING COMPLETE. TOOK " << open_time << "ms" << std::endl;
			}

			{
				std::cout << "[INFO] REALSENSE DEVICES CAPTURING" << std::endl;
				const std::chrono::steady_clock::time_point capture_start = std::chrono::steady_clock::now();
				std::vector<std::future<struct result>> capture_futures;
				capture_futures.reserve(device_count);
				for (uint32_t j = 0; j < device_count; j++)
				{
					capture_futures.emplace_back(std::async(std::launch::async, [&syncer_map](const uint32_t j)-> struct result
					{
						const rs2::syncer &syncer = syncer_map[j];
						rs2::frameset frameset;
						while (syncer.poll_for_frames(&frameset)) {} // Throw away old frames
						for (uint32_t k = 0; k < frame_capture_count; k++)
						{
							try
							{
								syncer.wait_for_frames(8000);
							}
							catch (const std::exception &)
							{
								return { j, EXIT_FAILURE };
							}
						}
						return { j, EXIT_SUCCESS };
					}, j));
				}
				for (const std::future<struct result> &capture_future : capture_futures)
				{
					capture_future.wait();
				}
				const std::chrono::steady_clock::time_point capture_stop = std::chrono::steady_clock::now();
				const time_t capture_time = std::chrono::duration_cast<std::chrono::milliseconds>(capture_stop - capture_start).count();
				std::cout << "[INFO] REALSENSE DEVICES CAPTURING COMPLETE. TOOK " << capture_time << "ms" << std::endl;
			}
			
			{
				std::cout << "[INFO] REALSENSE DEVICES CLOSING" << std::endl;
				const std::chrono::steady_clock::time_point close_start = std::chrono::steady_clock::now();
				std::vector<std::future<struct result>> close_futures;
				close_futures.reserve(device_count);
				for (uint32_t j = 0; j < device_count; j++)
				{
					close_futures.emplace_back(std::async(std::launch::async, [&cout_mutex, &color_sensor_map, &depth_sensor_map](const uint32_t j) -> struct result
					{
						const rs2::sensor &color_sensor = color_sensor_map[j];
						const rs2::sensor &depth_sensor = depth_sensor_map[j];

						const std::function<struct result(const rs2::sensor&)> stop = [&cout_mutex, &j](const rs2::sensor &sensor) -> struct result {
							try { sensor.stop(); }
							catch (const rs2::wrong_api_call_sequence_error &e)
							{
								std::lock_guard<std::mutex> _(cout_mutex);
								std::cout << "[ERROR] REALSENSE DEVICE " << j << " UNABLE TO STOP : " << e.what() << std::endl;
								return { j, EXIT_FAILURE };
							}
							return { j, EXIT_SUCCESS };
						};

						const std::function<struct result(const rs2::sensor&)> close = [&cout_mutex, &j](const rs2::sensor &sensor) -> struct result {
							try { sensor.close(); }
							catch (const rs2::wrong_api_call_sequence_error &e)
							{
								std::lock_guard<std::mutex> _(cout_mutex);
								std::cout << "[ERROR] REALSENSE DEVICE " << j << " UNABLE TO CLOSE : " << e.what() << std::endl;
								return { j, EXIT_FAILURE };
							}
							return { j, EXIT_SUCCESS };
						};

						const std::future<struct result> depth_stop = std::async(std::launch::async, stop, depth_sensor.as<rs2::sensor>());
						const std::future<struct result> color_stop = std::async(std::launch::async, stop, color_sensor.as<rs2::sensor>());
						depth_stop.wait();
						color_stop.wait();
						const std::future<struct result> depth_close = std::async(std::launch::async, close, depth_sensor.as<rs2::sensor>());
						const std::future<struct result> color_close = std::async(std::launch::async, close, color_sensor.as<rs2::sensor>());
						depth_close.wait();
						color_close.wait();
						
						return { j, EXIT_SUCCESS };
					}, j));
				}
				// Verify All Color Sensor Closing
				for (const std::future<struct result> &close_future : close_futures)
				{
					close_future.wait();
				}
				const std::chrono::steady_clock::time_point close_stop = std::chrono::steady_clock::now();
				const time_t close_time = std::chrono::duration_cast<std::chrono::milliseconds>(close_stop - close_start).count();
				std::cout << "[INFO] REALSENSE DEVICES CLOSING COMPLETE. TOOK " << close_time << "ms" << std::endl;
			}
		}

		std::cout << "[INFO] Closing Realsense Sandbox Program. " << std::endl;
	}
	return EXIT_SUCCESS;
}