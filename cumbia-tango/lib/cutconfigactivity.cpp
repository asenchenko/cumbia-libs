#include "cutconfigactivity.h"
#include <tango.h>
#include <cumacros.h>
#include "cudevicefactoryservice.h"
#include "tdevice.h"
#include "cutango-world.h"

class CuTAttConfigActivityPrivate
{
public:
    CuDeviceFactoryService *device_service;
    TDevice *tdev;
    std::string msg;
    bool err;
    pthread_t my_thread_id, other_thread_id;
    bool exiting;
    int repeat, try_cnt;
    CuTConfigActivity::Type type;
    CuData options;
};

CuTConfigActivity::CuTConfigActivity(const CuData &tok, CuDeviceFactoryService *df, Type t) : CuActivity(tok)
{
    d = new CuTAttConfigActivityPrivate;
    d->device_service = df;
    d->type = t;
    d->tdev = NULL;
    d->err = false;
    d->other_thread_id = pthread_self();
    d->exiting = false;
    d->repeat = -1;
    d->try_cnt = 0;
    setFlag(CuActivity::CuAUnregisterAfterExec, true);
    setFlag(CuActivity::CuADeleteOnExit, true);
}

CuTConfigActivity::~CuTConfigActivity()
{
    pdelete("CuTAttConfigActivity %p", this);
    delete d;
}

void CuTConfigActivity::setOptions(const CuData &o) {
    d->options = o;
}

int CuTConfigActivity::getType() const
{
    return d->type;
}

void CuTConfigActivity::event(CuActivityEvent *e)
{
    (void )e;
}

bool CuTConfigActivity::matches(const CuData &token) const
{
    const CuData& mytok = getToken();
    return token["src"] == mytok["src"] && mytok["activity"] == token["activity"];
}

int CuTConfigActivity::repeat() const
{
    return -1;
}

void CuTConfigActivity::init()
{
    d->my_thread_id = pthread_self();
    assert(d->other_thread_id != d->my_thread_id);
    CuData tk = getToken();
    const std::string& dnam = tk["device"].toString();
    /* get a TDevice */
    d->tdev = d->device_service->getDevice(dnam, threadToken());
    // thread safe: since cumbia 1.1.0 no thread per device guaranteed
    d->device_service->addRef(dnam, threadToken());
    tk["msg"] =  "CuTConfigActivity.init: " + d->tdev->getMessage();
    tk["conn"] = d->tdev->isValid();
    tk["err"] = !d->tdev->isValid();
    tk.putTimestamp();
}

void CuTConfigActivity::execute()
{
    assert(d->my_thread_id == pthread_self());
    CuData at = getToken(); /* activity token */
    d->err = !d->tdev->isValid();
    std::string point = at["point"].toString();
    bool cmd = at["is_command"].toBool();
    at["properties"] = std::vector<std::string>();
    at["type"] = "property";
    bool value_only = d->options["value-only"].toBool();

    d->try_cnt++;
    bool success = false;

    if(d->tdev->isValid()) {
        Tango::DeviceProxy *dev = d->tdev->getDevice();
        CuTangoWorld utils;
        utils.fillThreadInfo(at, this); /* put thread and activity addresses as info */
        if(dev && cmd)
        {
            success = value_only || utils.get_command_info(dev, point, at);
            if(success && d->type == CuReaderConfigActivityType) {
                success = utils.cmd_inout(dev, point, at);
            }
        }
        else if(dev)  {
            value_only ? success = utils.read_att(dev, point, at)  : success = utils.get_att_config(dev, point, at);
        }
        else
            d->msg = d->tdev->getMessage();

        //
        // fetch attribute properties
        if(d->options.containsKey("fetch_props")) {
            const std::vector<std::string> &props = d->options["fetch_props"].toStringVector();
            if(props.size() > 0 && success && dev)
                success = utils.get_att_props(dev, point, at, props);
        }
        //

        at["data"] = true;
        at["msg"] = "CuTConfigActivity.execute: " + utils.getLastMessage();
        at["err"] = utils.error();
        d->err = utils.error();
        d->msg = utils.getLastMessage();

        // retry?
        d->err ?  d->repeat = 2000 * d->try_cnt : d->repeat = -1;
    }
    else {
        at["msg"] = "CuTConfigActivity.execute: " + d->tdev->getMessage();
        at["err"] = true;
        at.putTimestamp();
    }
    publishResult(at);
}

void CuTConfigActivity::onExit()
{
    assert(d->my_thread_id == pthread_self());
    CuData at = getToken(); /* activity token */
    if(!d->exiting) {
        d->exiting = true;
        int refcnt = -1;
        // thread safe remove ref and disposal
        refcnt = d->device_service->removeRef(at["device"].toString(), threadToken());
        if(!refcnt)
            d->tdev = nullptr;
    }
    at["exit"] = true;
    publishResult(at);
}
