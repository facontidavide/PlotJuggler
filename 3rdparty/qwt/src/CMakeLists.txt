
set( QWT_SRC
    qwt.h
    qwt_abstract_scale_draw.h
    qwt_bezier.h
    qwt_clipper.h
    qwt_color_map.h
    qwt_column_symbol.h
    qwt_date.h
    qwt_date_scale_draw.h
    qwt_date_scale_engine.h
    qwt_dyngrid_layout.h
    qwt_global.h
    qwt_graphic.h
    qwt_interval.h
    qwt_interval_symbol.h
    qwt_math.h
    qwt_magnifier.h
    qwt_null_paintdevice.h
    qwt_painter.h
    qwt_painter_command.h
    qwt_panner.h
    qwt_picker.h
    qwt_picker_machine.h
    qwt_pixel_matrix.h
    qwt_point_3d.h
    qwt_point_polar.h
    qwt_round_scale_draw.h
    qwt_scale_div.h
    qwt_scale_draw.h
    qwt_scale_engine.h
    qwt_scale_map.h
    qwt_spline_basis.h
    qwt_spline.h
    qwt_spline_parametrization.h
    qwt_spline_local.h
    qwt_spline_cubic.h
    qwt_spline_pleasing.h
    qwt_spline_polynomial.h
    qwt_symbol.h
    qwt_system_clock.h
    qwt_text_engine.h
    qwt_text_label.h
    qwt_text.h
    qwt_transform.h
    qwt_widget_overlay.h

    qwt_curve_fitter.h
    qwt_spline_curve_fitter.h
    qwt_weeding_curve_fitter.h
    qwt_event_pattern.h
    qwt_abstract_legend.h
    qwt_legend.h
    qwt_legend_data.h
    qwt_legend_label.h
    qwt_plot.h
    qwt_plot_renderer.h
    qwt_plot_curve.h
    qwt_plot_dict.h
    qwt_plot_directpainter.h
    qwt_plot_grid.h
    qwt_plot_histogram.h
    qwt_plot_item.h
    qwt_plot_abstract_barchart.h
    qwt_plot_barchart.h
    qwt_plot_multi_barchart.h
    qwt_plot_intervalcurve.h
    qwt_plot_tradingcurve.h
    qwt_plot_layout.h
    qwt_plot_marker.h
    qwt_plot_zoneitem.h
    qwt_plot_textlabel.h
    qwt_plot_rasteritem.h
    qwt_plot_spectrogram.h
    qwt_plot_spectrocurve.h
    qwt_plot_scaleitem.h
    qwt_plot_legenditem.h
    qwt_plot_seriesitem.h
    qwt_plot_shapeitem.h
    qwt_plot_canvas.h
    qwt_plot_panner.h
    qwt_plot_picker.h
    qwt_plot_zoomer.h
    qwt_plot_magnifier.h
    qwt_plot_rescaler.h
    qwt_point_mapper.h
    qwt_raster_data.h
    qwt_matrix_raster_data.h
    qwt_sampling_thread.h
    qwt_samples.h
    qwt_series_data.h
    qwt_series_store.h
    qwt_point_data.h
    qwt_scale_widget.h

    qwt.cpp
    qwt_curve_fitter.cpp
    qwt_bezier.cpp
    qwt_spline_curve_fitter.cpp
    qwt_weeding_curve_fitter.cpp
    qwt_abstract_legend.cpp
    qwt_legend.cpp
    qwt_legend_data.cpp
    qwt_legend_label.cpp
    qwt_plot.cpp
    qwt_plot_renderer.cpp
    qwt_plot_axis.cpp
    qwt_plot_curve.cpp
    qwt_plot_dict.cpp
    qwt_plot_directpainter.cpp
    qwt_plot_grid.cpp
    qwt_plot_histogram.cpp
    qwt_plot_item.cpp
    qwt_plot_abstract_barchart.cpp
    qwt_plot_barchart.cpp
    qwt_plot_multi_barchart.cpp
    qwt_plot_intervalcurve.cpp
    qwt_plot_zoneitem.cpp
    qwt_plot_tradingcurve.cpp
    qwt_plot_spectrogram.cpp
    qwt_plot_spectrocurve.cpp
    qwt_plot_scaleitem.cpp
    qwt_plot_legenditem.cpp
    qwt_plot_seriesitem.cpp
    qwt_plot_shapeitem.cpp
    qwt_plot_marker.cpp
    qwt_plot_textlabel.cpp
    qwt_plot_layout.cpp
    qwt_plot_canvas.cpp
    qwt_plot_panner.cpp
    qwt_plot_rasteritem.cpp
    qwt_plot_picker.cpp
    qwt_plot_zoomer.cpp
    qwt_plot_magnifier.cpp
    qwt_plot_rescaler.cpp
    qwt_point_mapper.cpp
    qwt_raster_data.cpp
    qwt_matrix_raster_data.cpp
    qwt_sampling_thread.cpp
    qwt_series_data.cpp
    qwt_point_data.cpp
    qwt_scale_widget.cpp
    qwt_abstract_scale_draw.cpp
    qwt_clipper.cpp
    qwt_color_map.cpp
    qwt_column_symbol.cpp
    qwt_date.cpp
    qwt_date_scale_draw.cpp
    qwt_date_scale_engine.cpp
    qwt_dyngrid_layout.cpp
    qwt_event_pattern.cpp
    qwt_graphic.cpp
    qwt_interval.cpp
    qwt_interval_symbol.cpp
    qwt_math.cpp
    qwt_magnifier.cpp
    qwt_null_paintdevice.cpp
    qwt_painter.cpp
    qwt_painter_command.cpp
    qwt_panner.cpp
    qwt_picker.cpp
    qwt_picker_machine.cpp
    qwt_pixel_matrix.cpp
    qwt_point_3d.cpp
    qwt_point_polar.cpp
    qwt_round_scale_draw.cpp
    qwt_scale_div.cpp
    qwt_scale_draw.cpp
    qwt_scale_map.cpp
    qwt_scale_engine.cpp
    qwt_spline_basis.cpp
    qwt_spline.cpp
    qwt_spline_parametrization.cpp
    qwt_spline_local.cpp
    qwt_spline_cubic.cpp
    qwt_spline_pleasing.cpp
    qwt_symbol.cpp
    qwt_system_clock.cpp
    qwt_text_engine.cpp
    qwt_text_label.cpp
    qwt_text.cpp
    qwt_transform.cpp
    qwt_widget_overlay.cpp

    qwt_plot_abstract_canvas.h
    qwt_plot_abstract_canvas.cpp

    qwt_plot_opengl_canvas.h
    qwt_plot_opengl_canvas.cpp
    )

add_library(plotjuggler_qwt STATIC ${QWT_SRC})

target_include_directories(plotjuggler_qwt PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(plotjuggler_qwt
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Svg
    )

target_compile_definitions(plotjuggler_qwt PUBLIC QWT_MOC_INCLUDE)
#target_compile_definitions(plotjuggler_qwt PUBLIC QWT_DLL QWT_MAKEDLL )

add_library(plotjuggler_qwt_objects OBJECT ${QWT_SRC})
target_compile_definitions(plotjuggler_qwt_objects PUBLIC QWT_MOC_INCLUDE)

install(
     TARGETS
        plotjuggler_qwt
     EXPORT export_plotjuggler_qwt
     LIBRARY DESTINATION lib
     ARCHIVE DESTINATION lib
     RUNTIME DESTINATION bin
     INCLUDES DESTINATION include )

