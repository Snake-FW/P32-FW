#pragma once
#include <functional>
#include "puppies/BootloaderProtocol.hpp"
#include "unique_file_ptr.hpp"
#include <span>
#include <puppies/puppy_constants.hpp>
#include <bit>

namespace buddy::puppies {

/**
 * @brief Start sequence of puppy boards (Detect, Flash, start application)
 *
 */
class PuppyBootstrap {
public:
    /// GUI callback enum
    enum class FlashingStage {
        START,                 ///< Reset puppies
        DISCOVERY,             ///< and assign them addresses
        CALCULATE_FINGERPRINT, ///< Puppies are calculating fingerprints
        CHECK_FINGERPRINT,     ///< Check match of fingerprint
        FLASHING,              ///< Writing firmware
        DONE,                  ///< Starting app
    };

    struct Progress {
        int percent_done;
        FlashingStage stage;
        PuppyType puppy_type;

        const char *description();
    };

    /// Callback to GUI for displaying process
    using ProgressHook = std::function<void(Progress progress)>;

    /// Minimal puppy bootloader version that works with this bootstrap
    static constexpr uint32_t MINIMAL_BOOTLOADER_VERSION = 294;

    /// @brief Result of puppy bootstrap - indicates which kennels are occupied
    struct BootstrapResult {
        uint8_t kennels_preset { 0 }; // every bit corresponds with one kennel

        // number of detected puppies
        [[nodiscard]] uint8_t discovered_num() const {
            return std::popcount(kennels_preset);
        }

        /// sets that this kennel is occupied
        void set_kennel_occupied(Kennel k) {
            kennels_preset |= 1 << static_cast<uint8_t>(k);
        }

        /// checks if kennel is occupied
        [[nodiscard]] bool is_kennel_occupied(Kennel k) const {
            return kennels_preset & (1 << static_cast<uint8_t>(k));
        }
    };

    /// Constructor
    PuppyBootstrap(ProgressHook progressHook);

    /// Start bootstrap procedure
    BootstrapResult run(PuppyBootstrap::BootstrapResult minimal_config, unsigned int max_attempts = 3);

    /// @brief  Returns address on RS485 for bootloader protocol for each kennel
    static constexpr BootloaderProtocol::Address get_boot_address_for_kennel(Kennel kennel) {
        return (BootloaderProtocol::Address)((uint8_t)BootloaderProtocol::Address::FIRST_ASSIGNED + (uint8_t)kennel);
    }

    /// @brief  Returns address on RS485 for modbus protocol for each kennel
    static constexpr BootloaderProtocol::Address get_modbus_address_for_kennel(Kennel kennel) {
        return (BootloaderProtocol::Address)((uint8_t)BootloaderProtocol::Address::MODBUS_OFFSET + (uint8_t)kennel);
    }

    /// @brief  This is minimal puppy configuration that is needed for printer to boot up. Minimal puppy config is that we have modular bed & dwarf 1
    static constexpr inline BootstrapResult MINIMAL_PUPPY_CONFIG {
        1 << static_cast<uint8_t>(Kennel::MODULAR_BED) | 1 << static_cast<uint8_t>(Kennel::DWARF_1)
    };

private:
    using fingerprint_t = BootloaderProtocol::fingerprint_t;

    /// Helper to index fingerprints by the kennel
    class fingerprints_t {
        std::array<fingerprint_t, static_cast<uint8_t>(Kennel::LAST) + 1> fingerprints;
        std::array<uint32_t, static_cast<uint8_t>(Kennel::LAST) + 1> salts;

    public:
        uint32_t &get_salt(Kennel kennel) {
            return salts[static_cast<uint8_t>(kennel)];
        }

        fingerprint_t &get_fingerprint(Kennel kennel) {
            return fingerprints[static_cast<uint8_t>(kennel)];
        }
    };

    BootloaderProtocol flasher;
    ProgressHook progressHook; // Hook for bootstrap progress bar, expecting percentages from 0 - 100, which will be adjusted in guimain to fit max_bootstrap_perc constant
    void reset_all_puppies();
    void reset_puppies_range(Kennel from, Kennel to);

    /**
     * @brief Test if puppy bootloader is there and check some info.
     *
     * @param type expecting this type of puppy
     * @param address check puppy with this modbus address
     */
    bool discover(PuppyType type, BootloaderProtocol::Address address);
    unique_file_ptr get_firmware(PuppyType type);
    off_t get_firmware_size(PuppyType type);

    /**
     * @brief Check fingerprint and if needed, flash new firmware.
     * @param kennel check puppy in this kennel
     * @param fw_fingerprints salts already given to puppies and each corresponding fingerprint
     * @param chunk_offset do initial fingerprint check only with chunk starting at offset
     * @param chunk_size do initial fingerprint check only with chunk of this size bytes
     * @param percent_offset start position of the progress trackbar
     * @param percent_span length on the progress trackbar filled with this check
     */
    void flash_firmware(Kennel kennel, fingerprints_t &fw_fingerprints, uint8_t chunk_offset, uint8_t chunk_size, int percent_offset, int percent_span);

    /**
     * @brief Tell puppy to check fingerprint and start application.
     * @param type not used now
     * @param address puppy's modbus address
     * @param salt use this salt for fingerprint calculation
     * @param fingerprint puppy will check this fingerprint before starting the app
     */
    void start_app(PuppyType type, BootloaderProtocol::Address address, uint32_t salt, const fingerprint_t &fingerprint);

    /**
     * @brief Wait for puppy to finish fingerprint calculation.
     * Puppy's address needs to be set by flasher.set_address(address) before calling this.
     * @param calculation_start time of ticks_ms() when the calculation was started.
     */
    void wait_for_fingerprint(uint32_t calculation_start);

    /**
     * @brief Calculate fingerprint of a puppy's firmware.
     * @param file this firmware
     * @param fw_size size of the fingerprinted area in bytes
     * @param fingerprint output fingerprint
     * @param salt add this salt before the app firmware
     */
    void calculate_fingerprint(unique_file_ptr &file, off_t fw_size, fingerprint_t &fingerprint, uint32_t salt);

    /**
     * @brief Check chunk of fingerprint from puppy.
     * @param fingerprint fingerprint to compare
     * @param offset compare only a chunk starting at offset
     * @param size compare only a chunk of size bytes
     * Offset and size allow to check only a chunk of the entire fingerprint.
     * It is used to check multiple puppies with one salt.
     * Second puppy cannot reuse a chunk from the first puppy that appeared on the bus.
     * @return true if fingerprint matches
     */
    bool fingerprint_match(const fingerprint_t &fingerprint, uint8_t offset = 0, uint8_t size = sizeof(fingerprint_t));

    BootstrapResult run_address_assignment();
    void assign_address(BootloaderProtocol::Address current_address, BootloaderProtocol::Address new_address);
    bool is_puppy_config_ok(BootstrapResult result, BootstrapResult minimal_config);
    void verify_address_assignment(BootstrapResult result);

    /**
     * @brief Tell puppies to start fingerprint calculation.
     * @param address puppy's address
     * @param salt add this salt into sha before the app
     */
    void start_fingerprint_computation(BootloaderProtocol::Address address, uint32_t salt);

    /**
     * @brief Downloads crash dump from a puppy if present.
     *
     * @return true if successfully downloaded a crash dump.
     */
    bool attempt_crash_dump_download(Kennel kennel, BootloaderProtocol::Address address);
};
}
