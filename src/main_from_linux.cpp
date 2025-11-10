// #include <Arduino.h>
// #include "bus.h"
// #include "cartridge.h"
// #include "cpu.h"
// #include "emulator_config.h"
// #include "mapper.h"
// #include "ppu.h"

// const double CLK_TIME = 1.0 / 1789773.0;
// uint64_t frame_duration = 1e9/60.0;
// int main() {
// 	// if (argc < 2) {
// 	// 	std::cerr << "Usage: " << argv[0] << "rom_name\n";
// 	// 	return -1;
// 	// }
// 	Screen screen;

// 	Config config = Config("smb.bin");
// 	CARTRIDGE cartridge(config);
// 	if (!cartridge.read_file()) exit(-1);

	
	
// 	// SDL sdl(screen, bus);
// 	// init_sdl(sdl, false);
// 	// sdl.state = PAUSED;
	
// 	while (1) {
// 		bus.controller[0] = controller_input_buffer;
// 		if (sdl.state == RUNNING) {
// 			uint32_t start_time = SDL_GetTicks();
// 			uint32_t end_time = 0;
// 			bus.clock(false);
// 			if (screen.RENDER_ENABLED == true) {
// 				// cout << "=======RENDERING FRAME=======" << frames << '\n';
// 				sdl.render_frame();
// 				frames++;
// 				uint32_t end_time = SDL_GetTicks(); 
// 				screen.RENDER_ENABLED = false;
// 				double time_elapsed = (end_time - start_time) * 1e6;
// 				if (time_elapsed < frame_duration) {
// 					//SDL_Delay(static_cast<uint32_t>(frame_duration - time_elapsed));
// 					timespec requested_time;
// 					timespec remaining_time;
// 					requested_time.tv_nsec = frame_duration - time_elapsed;
// 					//cout << requested_time.tv_nsec << '\n';
// 					nanosleep(&requested_time, &remaining_time);
// 					//cout << end_time << " " << start_time << std::endl;
// 				}
// 			}
// 			if (frames == 30) {
// 				std::cout << "ONE SECOND PASSED" << std::endl;
// 				frames = 0;
// 			}
// 		} else if (sdl.state == PAUSED) {
// 			if (sdl.tick == true) {
// 				std::string debug_output_log = bus.clock(true);
// 				// for (int i = 0; i < 3; i++) 
// 				// 	ppu.execute();
// 				// render_frmae
// 				if (screen.RENDER_ENABLED == true) {
// 					sdl.render_frame();
// 					screen.RENDER_ENABLED = false;
// 				}
// 				render_text(sdl, debug_output_log);
// 				sdl.tick = false;
// 			}
// 			continue;
// 		} else if (sdl.state == QUIT) {
// 			sdl.running = false;
// 			bus.hexdump();
// 			ppu.hexdump();
// 			cpu.hexdump();
// 			goto ENDLOOP;
// 		}
// 	}
// ENDLOOP:
// 	destroy_sdl(sdl);
// 	if(input_thread.joinable())
// 		input_thread.join();
// 	// ram.hexdump();

// 	// SDL_DestroyRenderer(renderer);
// 	// SDL_DestroyWindow(window);
// 	// SDL_Quit();
// 	// Assert assert;
// 	// assert.unit_test();
// }