#pragma once
#include <Arduino.h>

#include "hub_macro.hpp"

namespace ghc {

enum class Tag : uint8_t {
    api_v = 0,
    id,
    client,
    type,
    update,
    updates,
    get,
    last,
    crc32,
    discover,
    name,
    prefix,
    icon,
    PIN,
    version,
    max_upl,
    http_t,
    ota_t,
    ws_port,
    modules,
    total,
    used,
    code,
    OK,
    ack,
    info,
    controls,
    ui,
    files,
    notice,
    alert,
    push,
    script,
    refresh,
    print,

    error,
    fs_err,
    ota_next,
    ota_done,
    ota_err,
    fetch_start,
    fetch_chunk,
    fetch_err,
    upload_next,
    upload_done,
    upload_err,
    ota_url_err,
    ota_url_ok,

    value,
    maxlen,
    rows,
    regex,
    align,
    min,
    max,
    step,
    dec,
    unit,
    fsize,
    action,
    nolabel,
    suffix,
    notab,
    square,
    disable,
    hint,
    len,
    wwidth,
    wheight,
    data,
    func,
    keep,
    exp,

    plugin,
    js,
    css,
    ui_file,
    stream,
    port,
    canvas,
    width,
    height,
    active,
    html,
    dummy,
    menu,
    gauge,
    gauge_r,
    gauge_l,
    led,
    log,
    table,
    image,
    text,
    display,
    text_f,
    label,
    title,
    dpad,
    joy,
    flags,
    tabs,
    switch_t,
    switch_i,
    button,
    color,
    select,
    spinner,
    slider,
    datetime,
    date,
    time,
    confirm,
    prompt,
    area,
    pass,
    input,
    hook,
    row,
    col,
    space,
    platform,
};

}  // namespace ghc