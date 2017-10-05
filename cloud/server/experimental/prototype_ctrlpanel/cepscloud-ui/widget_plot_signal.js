function helper_make_plot_widget_content(widget_id, signal) {
    if (signal.value == undefined) return helper_make_widget_content_with_loader();
    let r = `
    <div>
        <canvas id="${widget_id}_plot_canvas"></canvas>
    </div>
    <div id="${widget_id}_plot_canvas_overview_container" class = "widget-plot-overview-container">
        <canvas id="${widget_id}_plot_canvas_overview" class = "widget-plot-overview-canvas"></canvas>
    </div>

    `;
    return r;
}

function setup_plot_signal_widget(widget_id, signal) {
    let widget_info = get_widget_info(widget_id);
    let widget = $("#" + widget_id);
    $(`#header_of_${widget_id}`).html(
        signal.name +
        `<button type="button" class="btn btn-xs close" aria-label="Close" onclick="close_widget(event,'${widget_id}');" style="transform: translateY(-11px);">
           <span aria-hidden="true" class="small">&times;</span>
         </button>`
    );
    $(`#${widget_id}`).removeClass("baseWidget-not-initialized");
    let widget_content = helper_make_plot_widget_content(widget_id, signal);
    $(`#content_of_${widget_id}`).html(widget_content);

    $(`#${widget_id}`).removeClass("depend-on-signal-data").addClass("depend-on-signal-data");
    $(`#${widget_id}`).removeClass("waiting-for-data");

    if (signal.value == undefined) {
        $(`#${widget_id}`).addClass("waiting-for-data");
        $(`#${widget_id}`).attr("attached-signal", signal.name);
        return;
    }

    var ctx = document.getElementById(widget_id + "_plot_canvas").getContext("2d");
    let config = {
        type: "line",
        animation: false,
        data: {
            datasets: [{
                data : []
            }]
        },
        options: {
            events: [],
            //showLines: false ,
            animation: {
                duration: 0, // general animation time
            },
            hover: {
                animationDuration: 0, // duration of animations when hovering an item
            },
            responsiveAnimationDuration: 0,
            elements: {
                line: {
                    tension: 0, // disables bezier curves
                }
            }
        }

    };
    let chart = new Chart(ctx, config);
    if (widget_info.plot_config == undefined) {
        widget_info.plot_config = {
            time_res_ms: 10,
            update_on_data_change_only: false,
            points_in_view: 512,
            data: [],
            active_slice_start: -1,
            active_slice_end: 0,
            ticker: 0,
            update_delta: 40,
            plotted_data_slice: [],
            data_start : undefined
        };
    }
    let counter = 0;
    widget_info.plot_config.update = setInterval(
        () => {
            let v = signal.value;
            widget_info.plot_config.data.push(v);
            ++widget_info.plot_config.ticker;
            if (chart.data.labels.length < widget_info.plot_config.points_in_view) chart.data.labels.push("");
            if (widget_info.plot_config.active_slice_start < 0) {
                widget_info.plot_config.active_slice_start = 0;
                widget_info.plot_config.active_slice_end = 0;
            } else if (widget_info.plot_config.data_start == undefined) ++widget_info.plot_config.active_slice_end;
            if (widget_info.plot_config.ticker % widget_info.plot_config.update_delta != 0) return;
            widget_info.plot_config.ticker = 0;


            if (widget_info.plot_config.active_slice_start + widget_info.plot_config.points_in_view < widget_info.plot_config.active_slice_end) {
                widget_info.plot_config.active_slice_start = widget_info.plot_config.active_slice_end - widget_info.plot_config.points_in_view;
            }
            

            if (widget_info.plot_config.active_slice_start == 0) chart.data.datasets[0].data = widget_info.plot_config.data;
            else {
                //chart.data.datasets[0].data = widget_info.plot_config.data.slice(widget_info.plot_config.active_slice_start, widget_info.plot_config.active_slice_end);
                if (widget_info.plot_config.plotted_data_slice.length < widget_info.plot_config.active_slice_end - widget_info.plot_config.active_slice_start)
                    widget_info.plot_config.plotted_data_slice.length = widget_info.plot_config.active_slice_end - widget_info.plot_config.active_slice_start;
                for (let i = 0; i < widget_info.plot_config.active_slice_end - widget_info.plot_config.active_slice_start; ++i)
                    widget_info.plot_config.plotted_data_slice[i] = widget_info.plot_config.data[widget_info.plot_config.active_slice_start + i];
                chart.data.datasets[0].data = widget_info.plot_config.plotted_data_slice;
            }
            chart.update(0);
        },
        widget_info.plot_config.time_res_ms
    );


    var ctx_overview = document.getElementById(widget_id + "_plot_canvas_overview").getContext("2d");
    save_widget_info(widget, widget_info, false);
    let config_overview = {
        type: "line",
        animation: false,
        data: {
            datasets: [{
                pointRadius : 0,
                borderWidth : 0,
                data: []
                //borderColor: "black",
                //fill:false
                //lineTension: 1
            }]
        },
        options: {
            events: [],
            animation: {
                duration: 0, // general animation time
            },
            hover: {
                animationDuration: 0, // duration of animations when hovering an item
            },
            responsiveAnimationDuration: 0,
            elements: {
                line: {
                    tension: 0, // disables bezier curves
                }
            },
            showLines: true ,
            showScale: false,
            scales:
            {
                yAxes: [{
                    ticks: {
                        display: false
                    },
                    gridLines: {
                        drawBorder: false,
                        display: false
                    }
                }],
                xAxes: [{
                    ticks: {
                        display: false
                    },
                    gridLines: {
                        drawBorder: false,
                        display: false
                    }
                }]
            },

            legend: {
                display: false
            },
            tooltips: {
                enabled: false
            },
           
            maintainAspectRatio: false,

        }
    };



    let chart_overview = new Chart(ctx_overview, config_overview);
    (() => {
        let slider = undefined;
        let slider_width = undefined;
        let slider_height = undefined;
        let canvas_ofs = undefined;
        let canvas_width = undefined;
        let canvas_height = undefined;
        let data_points_covered_by_slider = widget_info.plot_config.points_in_view;
        let data_start = undefined;
        let slider_moving = false;

        function compute_slider_width() {
            return Math.max(10,canvas_width * (data_points_covered_by_slider / chart_overview.data.datasets[0].data.length));
        }


        let f = function () {
            chart_overview.data.datasets[0].data = widget_info.plot_config.data;
            for (; chart_overview.data.labels.length < widget_info.plot_config.data.length;)  chart_overview.data.labels.push("");

            let old_slider_width = slider_width;
            if (slider != undefined) {
                slider_width = compute_slider_width();
                $(slider).css("width", slider_width.toString() + "px");
            }
            canvas_ofs = $(`#${widget_id}_plot_canvas_overview`).offset();
            canvas_width = $(`#${widget_id}_plot_canvas_overview`).width();
            canvas_height = slider_height = $(`#${widget_id}_plot_canvas_overview`).height();

            if (slider == undefined && chart_overview.data.datasets[0].data.length > 1.25 * widget_info.plot_config.points_in_view) {
                $("#" + widget_id + "_plot_canvas_overview_container").append(
                    `<div id="${widget_id}_plot_canvas_overview_slider" class="plot-canvas-overview-slider" ></div>`
                );
                slider_width = compute_slider_width();
                slider = $(`#${widget_id}_plot_canvas_overview_slider`);

                $(slider).css("width", slider_width.toString() + "px");
                $(slider).css("height", canvas_height.toString() + "px");


                $(slider).offset({ top: canvas_ofs.top, left: canvas_ofs.left + canvas_width - slider_width });

                slider.mousedown((ev) => {
                    slider_moving = true;
                    let p = $(`#${widget_id}_plot_canvas_overview_slider`).offset();
                    let dx = ev.pageX - p.left;
                    let dy = ev.pageY - p.top;
                    canvas_ofs = $(`#${widget_id}_plot_canvas_overview`).offset();
                    canvas_width = $(`#${widget_id}_plot_canvas_overview`).width();
                    canvas_height = slider_height = $(`#${widget_id}_plot_canvas_overview`).height();

                    $(document).mousemove((ev) => {
                        let t = ev.pageY - dy;
                        if (t < canvas_ofs.top) t = canvas_ofs.top;
                        if (t + slider_height > canvas_ofs.top + canvas_height) t = canvas_ofs.top + canvas_height - slider_height;

                        let l = ev.pageX - dx;
                        if (l < canvas_ofs.left) l = canvas_ofs.left;
                        if (l + slider_width >= canvas_ofs.left + canvas_width - 10) {
                            l = canvas_ofs.left + canvas_width - slider_width;
                            data_start = undefined;
                            widget_info.plot_config.data_start = undefined;
                        } else {
                            data_start = Math.round(((l - canvas_ofs.left) / canvas_width) * widget_info.plot_config.data.length);
                            widget_info.plot_config.active_slice_start = data_start;
                            widget_info.plot_config.active_slice_end = Math.min(data_start + widget_info.plot_config.points_in_view, widget_info.plot_config.data.length);
                            widget_info.plot_config.data_start = data_start;
                        }

                        $(`#${widget_id}_plot_canvas_overview_slider`).offset({ top: t, left: l });
                       
                    });
                    $(document).mouseup((ev) => {
                        slider_moving = false;
                        $(document).unbind("mouseup").unbind("mousemove");
                    });
                });

            } else if (slider != undefined && !slider_moving) {
                if (data_start == undefined) {
                    if (old_slider_width != slider_width)
                        $(slider).offset({ top: canvas_ofs.top, left: canvas_ofs.left + canvas_width - slider_width });
                } else {
                    $(slider).offset({ top: canvas_ofs.top, left: canvas_ofs.left + canvas_width * (data_start / chart_overview.data.datasets[0].data.length ) });
                }
            }
            chart_overview.update(0);
            setTimeout(f, widget_info.plot_config.time_res_ms * (chart_overview.data.datasets[0].data.length / 1024) * 40);
        }
        setTimeout(f, widget_info.plot_config.time_res_ms * 40);
    })();
}