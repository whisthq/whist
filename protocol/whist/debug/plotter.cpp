
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file plotter.cpp
 * @brief function implementations for plotting graphs
 */

#include <whist/core/whist.h>

extern "C" {
#include "plotter.h"
#include <whist/utils/clock.h>
#include "whist/logging/logging.h"
#include "whist/utils/threads.h"
};

#include <unordered_map>
#include <deque>
#include <sstream>
#include <fstream>

// the data struct for sampling
typedef std::unordered_map<std::string, std::deque<std::pair<double, double>>> PlotData;

// The plotter is only for debug, use static global to make code simpler
static PlotData *g_plot_data;
static WhistMutex g_plot_mutex;

static std::atomic<bool> in_sampling = false;  // indicate if in the sampling status

/*
============================
Public Function Implementations
============================
*/
void whist_plotter_init() {
    if (g_plot_data != NULL) {
        return;
    }
    g_plot_data = new PlotData;
    g_plot_data->reserve(9999);
    g_plot_mutex = whist_create_mutex();
}

void whist_plotter_start_sampling() {
    whist_lock_mutex(g_plot_mutex);
    g_plot_data->clear();  // clear all previous data
    in_sampling = true;    // indicate sampling start
    whist_unlock_mutex(g_plot_mutex);
}

void whist_plotter_stop_sampling() {
    whist_lock_mutex(g_plot_mutex);
    in_sampling = false;  // indicate smapling stopped
    whist_unlock_mutex(g_plot_mutex);
}

// the whist_plotter_insert_sample() uses a string instead a int as label. So that you don't need a
// centralized logic to manage the mapping of id and string. It's optimized for easily use, instead
// of max performance.
void whist_plotter_insert_sample(const char *label, double x, double y) {
    if (!in_sampling) return;  // if not in sampling, this function returns immediately

    auto &plot_data = *g_plot_data;

    // for simplicity it uses a global lock, for debugging and analysis perpurse it's good enough.
    // If aimming for more performance in future, we can use one lock for each lable. We can even
    // consider lock free queues.
    whist_lock_mutex(g_plot_mutex);
    auto &q = plot_data[label];  // get the dataset with specific lable
    q.push_back({x, y});         // insert a sample
    whist_unlock_mutex(g_plot_mutex);
}

std::string whist_plotter_export() {
    std::stringstream ss;
    ss.precision(10);  // avoid loss precision

    auto &plot_data = *g_plot_data;

    // export all recorded samples to a json formated string
    ss << "{" << std::endl;

    whist_lock_mutex(g_plot_mutex);
    for (auto it = plot_data.begin(); it != plot_data.end(); it++) {
        auto &q = it->second;
        if (it != plot_data.begin()) {
            ss << "," << std::endl;
        }
        ss << "\"" << it->first << "\": [";
        for (auto it2 = q.begin(); it2 != q.end(); it2++) {
            if (it2 != q.begin()) {
                ss << ",";
            }
            ss << "[" << it2->first << "," << it2->second << "]";
        }
        ss << "]";
    }
    whist_unlock_mutex(g_plot_mutex);

    ss << "}" << std::endl;
    return ss.str();
}

void whist_plotter_export_to_file(const char *filename)
{
    std::string s=whist_plotter_export();
    std::ofstream myfile;
    myfile.open(filename);
    if(myfile.fail()){
        LOG_ERROR("open file %s for plotter export failed\n", filename);
        return;
    }
    myfile << s;
    myfile.close();
    LOG_INFO("Plotter data exported to file %s\n", filename);
}

void whist_plotter_export_c(char *out_s, size_t max_size) {
    std::string s = whist_plotter_export();
    strncpy(out_s, s.c_str(), min(max_size, s.size()));
}
