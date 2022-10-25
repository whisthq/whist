
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file plotter.cpp
 * @brief function implementations for plotting graphs
 */

#include <stdlib.h>
#include <whist/core/whist.h>
#include <memory>
#include <thread>

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

// Data struct to store samples
typedef std::unordered_map<std::string, std::deque<std::pair<double, double>>> PlotData;

// The class which serves as an interface of all plotter implementations
class PlotterInterface {
   protected:
    std::string type_name;

   public:
    virtual void insert_sample(const char *label, double x, double y) = 0;

    virtual void start_sampling() {
        LOG_FATAL("start_sampling() not implemented for %s", type_name.c_str());
    }
    virtual void stop_sampling() {
        LOG_FATAL("stop_sampling() not implemented for %s", type_name.c_str());
    }

    virtual int export_to_file(const char *filename) {
        LOG_FATAL("export_to_file() not implemented for %s", type_name.c_str());
    }
    virtual ~PlotterInterface() {}
};

// StreamPlotter peridically flushes samples onto the disk using the jsonline format
class StreamPlotter : public PlotterInterface {
    // Even-odd array for sample storage so insert and write operations don't block each other
    PlotData *plot_data_array[2];

    WhistMutex plot_mutex;
    std::atomic<bool> running;

    // Index of current sampling group which maps onto plot_data_array
    int current_idx;

    // Start time of current index
    double current_idx_start_time;

    std::ofstream plot_file;
    std::thread periodic_flush_thread;

   public:
    StreamPlotter(const std::string &file_name) {
        type_name = "StreamPlotter";
        // initalization
        current_idx = 0;
        current_idx_start_time = get_timestamp_sec();
        for (int i = 0; i < 2; i++) {
            plot_data_array[i] = new PlotData;
            plot_data_array[i]->reserve(9999);
        }
        plot_mutex = whist_create_mutex();
        running = true;

        // open file for writing
        plot_file.open(file_name);
        if (plot_file.fail()) {
            LOG_FATAL("open file %s for saving plotter data failed\n", file_name.c_str());
        }

        // Start a thread for periodically flushing plot data onto disk
        periodic_flush_thread = std::thread([&]() {
            const double flush_interval_seconds = 2.0;
            while (running) {
                double current_time = get_timestamp_sec();
                if (current_time - current_idx_start_time > flush_interval_seconds) {
                    int previous_idx;

                    // Increment current_idx and then begin flushing the old index
                    whist_lock_mutex(plot_mutex);
                    previous_idx = current_idx++;
                    whist_unlock_mutex(plot_mutex);
                    current_idx_start_time = get_timestamp_sec();
                    // flush the group pointed by previous_idx to disk
                    flush_to_file(previous_idx);
                    plot_data_array[previous_idx % 2]->clear();

                } else {
                    // Since this is only for debugging, it's fine to just use a sleep loop
                    whist_sleep(50);
                }
            }
            // Flush the last group to disk
            flush_to_file(current_idx);
        });
    }

    void insert_sample(const char *label, double x, double y) override {
        if (!running) return;
        whist_lock_mutex(plot_mutex);
        auto &plot_data = *plot_data_array[current_idx % 2];
        auto &q = plot_data[label];
        q.push_back({x, y});
        whist_unlock_mutex(plot_mutex);
    }

    ~StreamPlotter() override {
        // Break and join the processor thread
        running = false;
        periodic_flush_thread.join();
        plot_file.close();

        for (int i = 0; i < 2; i++) {
            delete plot_data_array[i];
        }
        whist_destroy_mutex(plot_mutex);
    }

   private:
    void flush_to_file(int idx) {
        auto &plot_data = *plot_data_array[idx % 2];
        std::stringstream ss;
        // Avoid loss of precision
        ss.precision(12);

        // Export all recorded samples to a json-line formated line
        ss << "{";

        for (auto it = plot_data.begin(); it != plot_data.end(); it++) {
            auto &q = it->second;
            if (it != plot_data.begin()) {
                ss << ",";
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
        ss << "}" << std::endl;
        plot_file << ss.str();
        plot_file.flush();
    }
};

class BasicPlotter : public PlotterInterface {
    PlotData *plot_data_ptr;
    WhistMutex plot_mutex;

    // Whether or not we are currently sampling
    std::atomic<bool> in_sampling;

   public:
    BasicPlotter() {
        type_name = "BasicPlotter";
        in_sampling = false;
        plot_data_ptr = new PlotData;
        plot_data_ptr->reserve(9999);
        plot_mutex = whist_create_mutex();
    }
    ~BasicPlotter() override {
        stop_sampling();
        delete plot_data_ptr;
        whist_destroy_mutex(plot_mutex);
    }

    void start_sampling() override {
        whist_lock_mutex(plot_mutex);
        plot_data_ptr->clear();  // clear all previous data
        in_sampling = true;      // indicate sampling start
        whist_unlock_mutex(plot_mutex);
    }

    void stop_sampling() override {
        whist_lock_mutex(plot_mutex);
        in_sampling = false;  // indicate sampling stopped
        whist_unlock_mutex(plot_mutex);
    }

    void insert_sample(const char *label, double x, double y) override {
        if (!in_sampling) return;  // if not in sampling, this function returns immediately
        auto &plot_data = *plot_data_ptr;

        // for simplicity it uses a global lock, for debugging and analysis perpurse it's good
        // enough. If aimming for more performance in future, we can use one lock for each label. We
        // can even consider lock free queues.
        whist_lock_mutex(plot_mutex);
        auto &q = plot_data[label];  // get the dataset with specific label
        q.push_back({x, y});         // insert a sample
        whist_unlock_mutex(plot_mutex);
    }

    std::string export_as_string() {
        std::stringstream ss;
        ss.precision(12);  // avoid loss precision

        auto &plot_data = *plot_data_ptr;

        // export all recorded samples to a json formated string with pretty print
        ss << "{" << std::endl;

        whist_lock_mutex(plot_mutex);
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
        whist_unlock_mutex(plot_mutex);

        ss << "}" << std::endl;
        return ss.str();
    }

    int export_to_file(const char *filename) override {
        std::string s = export_as_string();
        std::ofstream myfile;
        myfile.open(filename);
        if (myfile.fail()) {
            LOG_ERROR("open file %s for plotter export failed\n", filename);
            return -1;
        }
        myfile << s;
        myfile.close();
        LOG_INFO("Plotter data exported to file %s\n", filename);
        return 0;
    }
};

// The plotter is similar to the logger, which only has a single instance.
// Hence, we can use a static global singleton.

// Long-term ownership of the plotter
std::shared_ptr<PlotterInterface> g_plotter_ptr;

// Thread-safe access to the above shared_ptr.
// Note: accessing a single shared_ptr from two threads is not thread-safe,
// it's thread-safe to get a copy of shared_ptr with weak_ptr's lock() method
// from another thread, then do concurrent access
std::weak_ptr<PlotterInterface> g_plotter_weak_ptr;
static std::atomic<bool> initalized = false;

/*
============================
Public Function Implementations
============================
*/
void whist_plotter_init(const char *file_name) {
    if (initalized) {
        LOG_FATAL("whist_plotter_init is called twice");
    }
    if (file_name) {
        g_plotter_ptr = std::make_shared<StreamPlotter>(file_name);
    } else {
        g_plotter_ptr = std::make_shared<BasicPlotter>();
    }
    // let the weak_ptr point to shared_ptr
    g_plotter_weak_ptr = g_plotter_ptr;
    initalized = true;
}

void whist_plotter_start_sampling() {
    std::shared_ptr<PlotterInterface> local_ptr = g_plotter_weak_ptr.lock();
    FATAL_ASSERT(local_ptr);
    // Note: this function is only implemented for BasicPlotter
    local_ptr->start_sampling();
}

void whist_plotter_stop_sampling() {
    std::shared_ptr<PlotterInterface> local_ptr = g_plotter_weak_ptr.lock();
    FATAL_ASSERT(local_ptr);
    // Note: this function is only implemented for BasicPlotter
    if (!local_ptr) local_ptr->stop_sampling();
}

// We use a string instead of an int as a lable so that we don't need to build an ID-to-string
// mapping. This is optimized for ease of use instead of performance, which is fine for a debug
// tool.
void whist_plotter_insert_sample(const char *label, double x, double y) {
    // This function is a heavily called function.
    // we use the atomic<bool> to quit fast if the plotter is not initalized at all.
    // since the weak ptr's lock operation might be costly (depending on implementation).
    if (!initalized) return;

    std::shared_ptr<PlotterInterface> local_ptr = g_plotter_weak_ptr.lock();
    if (!local_ptr) return;
    local_ptr->insert_sample(label, x, y);
}

int whist_plotter_export_to_file(const char *filename) {
    std::shared_ptr<PlotterInterface> local_ptr = g_plotter_weak_ptr.lock();
    FATAL_ASSERT(local_ptr);
    // Note: this function is only implemented for BasicPlotter
    return local_ptr->export_to_file(filename);
}

void whist_plotter_destroy() {
    initalized = false;
    // release the long-term ownship of plotter. the plotter will be automatically destroyed when
    // all temp ownership is released.
    g_plotter_ptr.reset();
}
