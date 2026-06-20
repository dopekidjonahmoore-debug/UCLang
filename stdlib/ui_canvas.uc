# UI Canvas system with anchor layout support
# Anchors: "topleft"(default), "top", "topright", "left", "center", "right", "bottomleft", "bottom", "bottomright", "stretch"

ui_next_id == 0.
ui_canvases == [].
ui_screen_w == 1024.
ui_screen_h == 768.

def ui_set_screen_size(w, h.
    ui_screen_w == w.
    ui_screen_h == h.
).

# Apply anchor to get absolute position from relative (0-1) coordinates
def ui_apply_anchor(anchor, rx, ry, rw, rh.
    sw == ui_screen_w.
    sh == ui_screen_h.
    ax == rx.
    ay == ry.
    aw == rw.
    ah == rh.
    # Convert relative to absolute based on anchor
    # "center" means (rx, ry) is the center point
    If(anchor = "center" then(
        ax == rx - rw / 2.
        ay == ry - rh / 2.
    ). ).
    # "right" means right edge = rx
    If(anchor = "right" or anchor = "topright" or anchor = "bottomright" then(
        ax == rx - rw.
    ). ).
    # "bottom" means bottom edge = ry
    If(anchor = "bottom" or anchor = "bottomleft" or anchor = "bottomright" then(
        ay == ry - rh.
    ). ).
    # "stretch" fills available space
    If(anchor = "stretch" then(
        ax == 0.
        ay == 0.
        aw == sw.
        ah == sh.
    ). ).
    response([ax, ay, aw, ah]).
).

def ui_canvas_create(.
    id == ui_next_id.
    ui_next_id == id + 1.
    ui_canvases == list_push(ui_canvases, [id, [], 1]).
    response(id).
).

def ui_canvas_destroy(canvas_id.
    i == 0.
    While(i < list_len(ui_canvases) then(
        If(list_get(list_get(ui_canvases, i), 0) = canvas_id then(
            ui_canvases == list_remove(ui_canvases, i).
            i == list_len(ui_canvases).
        ). ).
        i == i + 1.
    ). ).
).

def ui_canvas_set_active(canvas_id, active.
    i == 0.
    While(i < list_len(ui_canvases) then(
        c == list_get(ui_canvases, i).
        If(list_get(c, 0) = canvas_id then(
            c == list_set(c, 2, active).
            ui_canvases == list_set(ui_canvases, i, c).
            i == list_len(ui_canvases).
        ). ).
        i == i + 1.
    ). ).
).

def ui_next_elem_id(.
    id == ui_next_id.
    ui_next_id == id + 1.
    response(id).
).

def ui_add_elem(canvas_id, elem_type, anchor, x, y, w, h, color, text, extra.
    elem_id == ui_next_elem_id().
    # Convert to absolute position based on anchor
    pos == ui_apply_anchor(anchor, x, y, w, h).
    ax == list_get(pos, 0).
    ay == list_get(pos, 1).
    aw == list_get(pos, 2).
    ah == list_get(pos, 3).
    i == 0.
    While(i < list_len(ui_canvases) then(
        c == list_get(ui_canvases, i).
        If(list_get(c, 0) = canvas_id then(
            elems == list_get(c, 1).
            elems == list_push(elems, [elem_id, elem_type, ax, ay, aw, ah, color, text, extra, 1, 0, anchor, x, y]).
            c == list_set(c, 1, elems).
            ui_canvases == list_set(ui_canvases, i, c).
            i == list_len(ui_canvases).
        ). ).
        i == i + 1.
    ). ).
    response(elem_id).
).

def ui_find_elem(elem_id.
    ci == 0.
    While(ci < list_len(ui_canvases) then(
        c == list_get(ui_canvases, ci).
        elems == list_get(c, 1).
        ei == 0.
        While(ei < list_len(elems) then(
            e == list_get(elems, ei).
            If(list_get(e, 0) = elem_id then(
                response([ci, ei, c, e]).
            ). ).
            ei == ei + 1.
        ). ).
        ci == ci + 1.
    ). ).
    response([-1, -1, [], []]).
).

def ui_image(canvas_id, anchor, x, y, w, h, color.
    response(ui_add_elem(canvas_id, "image", anchor, x, y, w, h, color, "", [])).
).

def ui_text(canvas_id, anchor, x, y, text, font_size, color.
    response(ui_add_elem(canvas_id, "text", anchor, x, y, 0, 0, color, text, [font_size])).
).

def ui_button(canvas_id, anchor, x, y, w, h, text, callback.
    response(ui_add_elem(canvas_id, "button", anchor, x, y, w, h, "#4488ff", text, [callback, 0])).
).

def ui_panel(canvas_id, anchor, x, y, w, h, color.
    response(ui_add_elem(canvas_id, "panel", anchor, x, y, w, h, color, "", [])).
).

def ui_slider(canvas_id, anchor, x, y, w, h, min_val, max_val, default_val.
    response(ui_add_elem(canvas_id, "slider", anchor, x, y, w, h, "#4488ff", "", [min_val, max_val, default_val, default_val])).
).

# Backward-compatible helpers (no anchor - defaults to "topleft")
def ui_image_old(canvas_id, x, y, w, h, color.
    response(ui_image(canvas_id, "topleft", x, y, w, h, color)).
).
def ui_text_old(canvas_id, x, y, text, font_size, color.
    response(ui_text(canvas_id, "topleft", x, y, text, font_size, color)).
).
def ui_button_old(canvas_id, x, y, w, h, text, callback.
    response(ui_button(canvas_id, "topleft", x, y, w, h, text, callback)).
).
def ui_panel_old(canvas_id, x, y, w, h, color.
    response(ui_panel(canvas_id, "topleft", x, y, w, h, color)).
).
def ui_slider_old(canvas_id, x, y, w, h, min_val, max_val, default_val.
    response(ui_slider(canvas_id, "topleft", x, y, w, h, min_val, max_val, default_val)).
).

def ui_set_text(elem_id, new_text.
    result == ui_find_elem(elem_id).
    ci == list_get(result, 0).
    ei == list_get(result, 1).
    c == list_get(result, 2).
    e == list_get(result, 3).
    If(ci >= 0 and ei >= 0 then(
        e == list_set(e, 7, new_text).
        elems == list_get(c, 1).
        elems == list_set(elems, ei, e).
        c == list_set(c, 1, elems).
        ui_canvases == list_set(ui_canvases, ci, c).
    ). ).
).

def ui_set_color(elem_id, new_color.
    result == ui_find_elem(elem_id).
    ci == list_get(result, 0).
    ei == list_get(result, 1).
    c == list_get(result, 2).
    e == list_get(result, 3).
    If(ci >= 0 and ei >= 0 then(
        e == list_set(e, 6, new_color).
        elems == list_get(c, 1).
        elems == list_set(elems, ei, e).
        c == list_set(c, 1, elems).
        ui_canvases == list_set(ui_canvases, ci, c).
    ). ).
).

def ui_set_pos(elem_id, x, y.
    result == ui_find_elem(elem_id).
    ci == list_get(result, 0).
    ei == list_get(result, 1).
    c == list_get(result, 2).
    e == list_get(result, 3).
    If(ci >= 0 and ei >= 0 then(
        e == list_set(e, 2, x).
        e == list_set(e, 3, y).
        elems == list_get(c, 1).
        elems == list_set(elems, ei, e).
        c == list_set(c, 1, elems).
        ui_canvases == list_set(ui_canvases, ci, c).
    ). ).
).

def ui_set_visible(elem_id, visible.
    result == ui_find_elem(elem_id).
    ci == list_get(result, 0).
    ei == list_get(result, 1).
    c == list_get(result, 2).
    e == list_get(result, 3).
    If(ci >= 0 and ei >= 0 then(
        e == list_set(e, 9, visible).
        elems == list_get(c, 1).
        elems == list_set(elems, ei, e).
        c == list_set(c, 1, elems).
        ui_canvases == list_set(ui_canvases, ci, c).
    ). ).
).

def ui_slider_value(elem_id.
    result == ui_find_elem(elem_id).
    ci == list_get(result, 0).
    ei == list_get(result, 1).
    e == list_get(result, 3).
    If(list_len(e) > 0 then(
        extra == list_get(e, 8).
        response(list_get(extra, 3)).
    ). ).
    response(0.0).
).

def ui_update_all(.
    mx == input.mouse_x().
    my == input.mouse_y().
    clicked == input.mouse_just_pressed(1).
    released == input.mouse_just_released(1).
    held == input.mouse(1).
    ci == 0.
    While(ci < list_len(ui_canvases) then(
        c == list_get(ui_canvases, ci).
        canvas_active == list_get(c, 2).
        If(canvas_active then(
            elems == list_get(c, 1).
            ei == 0.
            While(ei < list_len(elems) then(
                e == list_get(elems, ei).
                elem_visible == list_get(e, 9).
                If(elem_visible then(
                    elem_type == list_get(e, 1).
                    ex == list_get(e, 2).
                    ey == list_get(e, 3).
                    ew == list_get(e, 4).
                    eh == list_get(e, 5).
                    hover == mx >= ex and mx <= ex + ew and my >= ey and my <= ey + eh.
                    If(elem_type = "button" then(
                        extra == list_get(e, 8).
                        hover_prev == list_get(extra, 1).
                        extra == list_set(extra, 1, hover).
                        If(clicked and hover then(
                            cb_name == list_get(extra, 0).
                            run.call_func(cb_name).
                        ). ).
                        e == list_set(e, 8, extra).
                        elems == list_set(elems, ei, e).
                    ). ).
                    If(elem_type = "slider" then(
                        extra == list_get(e, 8).
                        min_val == list_get(extra, 0).
                        max_val == list_get(extra, 1).
                        cur_val == list_get(extra, 3).
                        If(held and hover then(
                            t == (mx - ex) / ew.
                            If(t < 0 then(t == 0. ). ).
                            If(t > 1 then(t == 1. ). ).
                            cur_val == min_val + t * (max_val - min_val).
                        ). ).
                        extra == list_set(extra, 3, cur_val).
                        e == list_set(e, 8, extra).
                        elems == list_set(elems, ei, e).
                    ). ).
                ). ).
                ei == ei + 1.
            ). ).
            c == list_set(c, 1, elems).
            ui_canvases == list_set(ui_canvases, ci, c).
        ). ).
        ci == ci + 1.
    ). ).
).

def ui_draw_all(.
    Overlay.begin().
    ci == 0.
    While(ci < list_len(ui_canvases) then(
        c == list_get(ui_canvases, ci).
        canvas_active == list_get(c, 2).
        If(canvas_active then(
            elems == list_get(c, 1).
            ei == 0.
            While(ei < list_len(elems) then(
                e == list_get(elems, ei).
                elem_visible == list_get(e, 9).
                If(elem_visible then(
                    elem_type == list_get(e, 1).
                    ex == list_get(e, 2).
                    ey == list_get(e, 3).
                    ew == list_get(e, 4).
                    eh == list_get(e, 5).
                    color == list_get(e, 6).
                    text == list_get(e, 7).
                    extra == list_get(e, 8).
                    If(elem_type = "image" then(
                        Overlay.rect(ex, ey, ew, eh, color).
                    ). ).
                    If(elem_type = "panel" then(
                        Overlay.rect(ex, ey, ew, eh, color).
                    ). ).
                    If(elem_type = "text" then(
                        fsize == list_get(extra, 0).
                        Overlay.text(ex, ey, text, color, fsize).
                    ). ).
                    If(elem_type = "button" then(
                        hover == list_get(extra, 1).
                        If(hover then(
                            Overlay.rect(ex - 2, ey - 2, ew + 4, eh + 4, "#66aaff").
                            Overlay.rect(ex, ey, ew, eh, "#4488ff").
                        ). else(
                            Overlay.rect(ex, ey, ew, eh, color).
                        ). ).
                        Overlay.rect(ex, ey, ew, eh, "#ffffff22").
                        tx == ex + ew / 2 - str_len(text) * 4.
                        ty == ey + eh / 2 - 8.
                        Overlay.text(tx, ty, text, "#ffffff", 16).
                    ). ).
                    If(elem_type = "slider" then(
                        min_val == list_get(extra, 0).
                        max_val == list_get(extra, 1).
                        cur_val == list_get(extra, 3).
                        t == 0.0.
                        If(max_val - min_val > 0 then(t == (cur_val - min_val) / (max_val - min_val). ). ).
                        track_h == 6.
                        ty2 == ey + (eh - track_h) / 2.
                        Overlay.rect(ex, ty2, ew, track_h, "#555566").
                        fill_w == t * ew.
                        If(fill_w > 4 then(Overlay.rect(ex, ty2, fill_w, track_h, "#4488ff"). ). ).
                        thumb_size == 14.
                        thumb_x == ex + t * ew - thumb_size / 2.
                        thumb_y == ey - (thumb_size - eh) / 2.
                        Overlay.rect(thumb_x, thumb_y, thumb_size, thumb_size, "#88bbff").
                        Overlay.rect(thumb_x, thumb_y, thumb_size, thumb_size, "#ffffff33").
                    ). ).
                ). ).
                ei == ei + 1.
            ). ).
        ). ).
        ci == ci + 1.
    ). ).
).
