#include <rtthread.h>

#include <sdl.h>
#include <rtdevice.h>
#include <rtgui/driver.h>

#define SDL_SCREEN_WIDTH    800
#define SDL_SCREEN_HEIGHT   480

struct sdlfb_device
{
    struct rt_device parent;

    SDL_Surface *screen;
    rt_uint16_t width;
    rt_uint16_t height;
};
struct sdlfb_device _device;

/* common device interface */
static rt_err_t  sdlfb_init(rt_device_t dev)
{
    return RT_EOK;
}
static rt_err_t  sdlfb_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}
static rt_err_t  sdlfb_close(rt_device_t dev)
{
    SDL_Quit();
    return RT_EOK;
}

static rt_mutex_t sdllock;
static rt_err_t  sdlfb_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    struct sdlfb_device *device;

    rt_mutex_take(sdllock, RT_WAITING_FOREVER);
    device = (struct sdlfb_device *)dev;
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->screen != RT_NULL);

    switch (cmd)
    {
    case RTGRAPHIC_CTRL_GET_INFO:
    {
        struct rt_device_graphic_info *info;

        info = (struct rt_device_graphic_info *) args;
        info->bits_per_pixel = 16;
        info->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565P;
        info->framebuffer = device->screen->pixels;
        info->width = device->screen->w;
        info->height = device->screen->h;
    }
    break;
    case RTGRAPHIC_CTRL_RECT_UPDATE:
    {
        struct rt_device_rect_info *rect;
        rect = (struct rt_device_rect_info *)args;

        /* SDL_UpdateRect(_device.screen, rect->x, rect->y, rect->x + rect->w, rect->y + rect->h); */
        SDL_UpdateRect(_device.screen, 0, 0, device->width, device->height);
    }
    break;
    case RTGRAPHIC_CTRL_SET_MODE:
    {
#if 0
        struct rt_device_rect_info *rect;

        rect = (struct rt_device_rect_info *)args;
        if ((_device.width == rect->width) && (_device.height == rect->height)) return -RT_ERROR;

        _device.width = rect->width;
        _device.height = rect->height;

        if (_device.screen != RT_NULL)
        {
            SDL_FreeSurface(_device.screen);

            /* re-create screen surface */
            _device.screen = SDL_SetVideoMode(_device.width, _device.height, 16, SDL_SWSURFACE | SDL_DOUBLEBUF);
            if (_device.screen == NULL)
            {
                fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
                exit(1);
            }

            SDL_WM_SetCaption("RT-Thread/GUI Simulator", NULL);
        }
#endif
    }
    break;
    }
    rt_mutex_release(sdllock);
    return RT_EOK;
}

static void sdlfb_hw_init(void)
{
    /* set video driver for VC++ debug */
    //_putenv("SDL_VIDEODRIVER=windib");

    //if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    _device.parent.init = sdlfb_init;
    _device.parent.open = sdlfb_open;
    _device.parent.close = sdlfb_close;
    _device.parent.read = RT_NULL;
    _device.parent.write = RT_NULL;
    _device.parent.control = sdlfb_control;

    _device.width  = SDL_SCREEN_WIDTH;
    _device.height = SDL_SCREEN_HEIGHT;
    _device.screen = SDL_SetVideoMode(_device.width, _device.height, 16, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (_device.screen == NULL)
    {
        fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("RT-Thread/GUI Simulator", NULL);
    rt_device_register(RT_DEVICE(&_device), "sdl", RT_DEVICE_FLAG_RDWR);

    sdllock = rt_mutex_create("fb", RT_IPC_FLAG_FIFO);
}

#include  <windows.h>
#include  <mmsystem.h>
#include  <stdio.h>
#include <sdl.h>
#include <rtgui/event.h>
#include <rtgui/kbddef.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>

static DWORD WINAPI sdl_loop(LPVOID lpParam)
{
    int quit = 0;
    SDL_Event event;
    int button_state = 0;

    rt_device_t device;
    sdlfb_hw_init();

    device = rt_device_find("sdl");
    rtgui_graphic_set_device(device);

    /* handle SDL event */
    while (!quit)
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_MOUSEMOTION:
#if  0
        {
            struct rtgui_event_mouse emouse;
            emouse.parent.type = RTGUI_EVENT_MOUSE_MOTION;
            emouse.parent.sender = RT_NULL;
            emouse.wid = RT_NULL;

            emouse.x = ((SDL_MouseMotionEvent *)&event)->x;
            emouse.y = ((SDL_MouseMotionEvent *)&event)->y;

            /* init mouse button */
            emouse.button = button_state;

            /* send event to server */
            rtgui_server_post_event(&emouse.parent, sizeof(struct rtgui_event_mouse));
        }
#endif
        break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            struct rtgui_event_mouse emouse;
            SDL_MouseButtonEvent *mb;

            emouse.parent.type = RTGUI_EVENT_MOUSE_BUTTON;
            emouse.parent.sender = RT_NULL;
            emouse.wid = RT_NULL;

            mb = (SDL_MouseButtonEvent *)&event;

            emouse.x = mb->x;
            emouse.y = mb->y;

            /* init mouse button */
            emouse.button = 0;

            /* set emouse button */
            if (mb->button & (1 << (SDL_BUTTON_LEFT - 1)))
            {
                emouse.button |= RTGUI_MOUSE_BUTTON_LEFT;
            }
            else if (mb->button & (1 << (SDL_BUTTON_RIGHT - 1)))
            {
                emouse.button |= RTGUI_MOUSE_BUTTON_RIGHT;
            }
            else if (mb->button & (1 << (SDL_BUTTON_MIDDLE - 1)))
            {
                emouse.button |= RTGUI_MOUSE_BUTTON_MIDDLE;
            }

            if (mb->type == SDL_MOUSEBUTTONDOWN)
            {
                emouse.button |= RTGUI_MOUSE_BUTTON_DOWN;
                button_state = emouse.button;
            }
            else
            {
                emouse.button |= RTGUI_MOUSE_BUTTON_UP;
                button_state = 0;
            }


            /* send event to server */
            rtgui_server_post_event(&emouse.parent, sizeof(struct rtgui_event_mouse));
        }
        break;

        case SDL_KEYUP:
        {
            struct rtgui_event_kbd ekbd;
            ekbd.parent.type    = RTGUI_EVENT_KBD;
            ekbd.parent.sender  = RT_NULL;
            ekbd.type = RTGUI_KEYUP;
            ekbd.wid = RT_NULL;
            ekbd.mod = event.key.keysym.mod;
            ekbd.key = event.key.keysym.sym;

            /* FIXME: unicode */
            ekbd.unicode = 0;

            /* send event to server */
            rtgui_server_post_event(&ekbd.parent, sizeof(struct rtgui_event_kbd));
        }
        break;

        case SDL_KEYDOWN:
        {
            struct rtgui_event_kbd ekbd;
            ekbd.parent.type    = RTGUI_EVENT_KBD;
            ekbd.parent.sender  = RT_NULL;
            ekbd.type = RTGUI_KEYDOWN;
            ekbd.wid = RT_NULL;
            ekbd.mod = event.key.keysym.mod;
            ekbd.key = event.key.keysym.sym;

            /* FIXME: unicode */
            ekbd.unicode = 0;

            /* send event to server */
            rtgui_server_post_event(&ekbd.parent, sizeof(struct rtgui_event_kbd));
        }
        break;

        case SDL_QUIT:
            SDL_Quit();
            quit = 1;
            break;

        default:
            break;
        }

        if (quit)
            break;
    }
    //exit(0);
    return 0;
}

/* start sdl thread */
void rt_hw_sdl_start(void)
{
    HANDLE thread;
    DWORD  thread_id;

    /* create thread that loop sdl event */
    thread = CreateThread(NULL,
                          0,
                          (LPTHREAD_START_ROUTINE)sdl_loop,
                          0,
                          CREATE_SUSPENDED,
                          &thread_id);
    if (thread == NULL)
    {
        //Display Error Message

        return;
    }
    ResumeThread(thread);
}
