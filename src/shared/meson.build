# SPDX-License-Identifier: LGPL-2.1-or-later

#if 0 /// elogind has a much shorter list
# shared_sources = files('''
#         acl-util.h
#         acpi-fpdt.c
#         acpi-fpdt.h
#         apparmor-util.c
#         apparmor-util.h
#         ask-password-api.c
#         ask-password-api.h
#         barrier.c
#         barrier.h
#         base-filesystem.c
#         base-filesystem.h
#         binfmt-util.c
#         binfmt-util.h
#         bitmap.c
#         bitmap.h
#         blkid-util.h
#         bond-util.c
#         bond-util.h
#         boot-timestamps.c
#         boot-timestamps.h
#         bootspec.c
#         bootspec.h
#         bpf-program.c
#         bpf-program.h
#         bridge-util.c
#         bridge-util.h
#         bus-get-properties.c
#         bus-get-properties.h
#         bus-locator.c
#         bus-locator.h
#         bus-log-control-api.c
#         bus-log-control-api.h
#         bus-map-properties.c
#         bus-map-properties.h
#         bus-message-util.c
#         bus-message-util.h
#         bus-object.c
#         bus-object.h
#         bus-polkit.c
#         bus-polkit.h
#         bus-print-properties.c
#         bus-print-properties.h
#         bus-unit-procs.c
#         bus-unit-procs.h
#         bus-unit-util.c
#         bus-unit-util.h
#         bus-util.c
#         bus-util.h
#         bus-wait-for-jobs.c
#         bus-wait-for-jobs.h
#         bus-wait-for-units.c
#         bus-wait-for-units.h
#         calendarspec.c
#         calendarspec.h
#         cgroup-setup.c
#         cgroup-setup.h
#         cgroup-show.c
#         cgroup-show.h
#         chown-recursive.c
#         chown-recursive.h
#         clean-ipc.c
#         clean-ipc.h
#         clock-util.c
#         clock-util.h
#         condition.c
#         condition.h
#         conf-parser.c
#         conf-parser.h
#         coredump-util.c
#         coredump-util.h
#         cpu-set-util.c
#         cpu-set-util.h
#         cryptsetup-util.c
#         cryptsetup-util.h
#         daemon-util.h
#         dev-setup.c
#         dev-setup.h
#         devnode-acl.h
#         discover-image.c
#         discover-image.h
#         dissect-image.c
#         dissect-image.h
#         dm-util.c
#         dm-util.h
#         dns-domain.c
#         dns-domain.h
#         dropin.c
#         dropin.h
#         efi-loader.c
#         efi-loader.h
#         enable-mempool.c
#         env-file-label.c
#         env-file-label.h
#         ethtool-util.c
#         ethtool-util.h
#         exec-util.c
#         exec-util.h
#         exit-status.c
#         exit-status.h
#         extension-release.c
#         extension-release.h
#         fdset.c
#         fdset.h
#         fileio-label.c
#         fileio-label.h
#         firewall-util-nft.c
#         firewall-util-private.h
#         firewall-util.c
#         firewall-util.h
#         format-table.c
#         format-table.h
#         fsck-util.h
#         fstab-util.c
#         fstab-util.h
#         generator.c
#         generator.h
#         geneve-util.c
#         geneve-util.h
#         gpt.c
#         gpt.h
#         group-record.c
#         group-record.h
#         hostname-setup.c
#         hostname-setup.h
#         id128-print.c
#         id128-print.h
#         idn-util.c
#         idn-util.h
#         ima-util.c
#         ima-util.h
#         import-util.c
#         import-util.h
#         initreq.h
#         install-printf.c
#         install-printf.h
#         install.c
#         install.h
#         ipvlan-util.c
#         ipvlan-util.h
#         ip-protocol-list.c
#         ip-protocol-list.h
#         journal-importer.c
#         journal-importer.h
#         journal-util.c
#         journal-util.h
#         json-internal.h
#         json.c
#         json.h
#         kbd-util.c
#         kbd-util.h
#         killall.c
#         killall.h
#         libcrypt-util.c
#         libcrypt-util.h
#         libfido2-util.c
#         libfido2-util.h
#         libmount-util.h
#         linux/auto_dev-ioctl.h
#         linux/bpf.h
#         linux/bpf_common.h
#         linux/bpf_insn.h
#         linux/dm-ioctl.h
#         linux/ethtool.h
#         local-addresses.c
#         local-addresses.h
#         lockfile-util.c
#         lockfile-util.h
#         log-link.h
#         logs-show.c
#         logs-show.h
#         loop-util.c
#         loop-util.h
#         loopback-setup.c
#         loopback-setup.h
#         machine-id-setup.c
#         machine-id-setup.h
#         machine-pool.c
#         machine-pool.h
#         macvlan-util.c
#         macvlan-util.h
#         main-func.h
#         mkfs-util.c
#         mkfs-util.h
#         module-util.h
#         mount-setup.c
#         mount-setup.h
#         mount-util.c
#         mount-util.h
#         net-condition.c
#         net-condition.h
#         netif-naming-scheme.c
#         netif-naming-scheme.h
#         nscd-flush.h
#         nsflags.c
#         nsflags.h
#         numa-util.c
#         numa-util.h
#         openssl-util.c
#         openssl-util.h
#         output-mode.c
#         output-mode.h
#         pager.c
#         pager.h
#         parse-argument.c
#         parse-argument.h
#         pe-header.h
#         pkcs11-util.c
#         pkcs11-util.h
#         pretty-print.c
#         pretty-print.h
#         psi-util.c
#         psi-util.h
#         ptyfwd.c
#         ptyfwd.h
#         pwquality-util.c
#         pwquality-util.h
#         qrcode-util.c
#         qrcode-util.h
#         reboot-util.c
#         reboot-util.h
#         resize-fs.c
#         resize-fs.h
#         resolve-util.c
#         resolve-util.h
#         seccomp-util.h
#         securebits-util.c
#         securebits-util.h
#         serialize.c
#         serialize.h
#         service-util.c
#         service-util.h
#         sleep-config.c
#         sleep-config.h
#         socket-netlink.c
#         socket-netlink.h
#         spawn-ask-password-agent.c
#         spawn-ask-password-agent.h
#         spawn-polkit-agent.c
#         spawn-polkit-agent.h
#         specifier.c
#         specifier.h
#         switch-root.c
#         switch-root.h
#         tmpfile-util-label.c
#         tmpfile-util-label.h
#         tomoyo-util.c
#         tomoyo-util.h
#         tpm2-util.c
#         tpm2-util.h
#         udev-util.c
#         udev-util.h
#         uid-range.c
#         uid-range.h
#         user-record-nss.c
#         user-record-nss.h
#         user-record-show.c
#         user-record-show.h
#         user-record.c
#         user-record.h
#         userdb.c
#         userdb.h
#         utmp-wtmp.h
#         varlink.c
#         varlink.h
#         verbs.c
#         verbs.h
#         vlan-util.c
#         vlan-util.h
#         volatile-util.c
#         volatile-util.h
#         watchdog.c
#         watchdog.h
#         web-util.c
#         web-util.h
#         wifi-util.c
#         wifi-util.h
#         xml.c
#         xml.h
# '''.split())
#else // 0
shared_sources = '''
        acl-util.h
        bootspec.c
        bootspec.h
        bus-get-properties.c
        bus-get-properties.h
        bus-locator.c
        bus-locator.h
        bus-log-control-api.c
        bus-log-control-api.h
        bus-map-properties.c
        bus-map-properties.h
        bus-object.c
        bus-object.h
        bus-polkit.c
        bus-polkit.h
        bus-print-properties.c
        bus-print-properties.h
        bus-util.c
        bus-util.h
        cgroup-setup.c
        cgroup-setup.h
        clean-ipc.c
        clean-ipc.h
        conf-parser.c
        conf-parser.h
        daemon-util.h
        dev-setup.c
        dev-setup.h
        devnode-acl.h
        dns-domain.c
        dns-domain.h
        efi-loader.c
        efi-loader.h
        enable-mempool.c
        exec-elogind.c
        exec-elogind.h
        exec-util.c
        exec-util.h
        fileio-label.c
        fileio-label.h
        format-table.c
        format-table.h
        group-record.c
        group-record.h
        json.c
        json.h
        json-internal.h
        main-func.h
        mount-setup.c
        mount-setup.h
        mount-util.h
        mount-util.c
        libcrypt-util.c
        libcrypt-util.h
        libmount-util.h
        nsflags.c
        nsflags.h
        openssl-util.h
        output-mode.c
        output-mode.h
        pager.c
        pager.h
        parse-argument.c
        parse-argument.h
        pe-header.h
        pkcs11-util.c
        pkcs11-util.h
        pretty-print.c
        pretty-print.h
        reboot-util.c
        reboot-util.h
        serialize.c
        serialize.h
        service-util.c
        service-util.h
        sleep-config.c
        sleep-config.h
        spawn-polkit-agent.c
        spawn-polkit-agent.h
        udev-util.c
        udev-util.h
        user-record-nss.c
        user-record-nss.h
        user-record.c
        user-record.h
        userdb.c
        userdb.h
        utmp-wtmp.h
        varlink.c
        varlink.h
        verbs.c
        verbs.h
        xml.c
        xml.h
'''.split()
#endif // 0

if get_option('tests') != 'false'
        shared_sources += files('''
                test-tables.h
                tests.c
                tests.h
        '''.split())
endif

generate_syscall_list = find_program('generate-syscall-list.py')
fname = 'syscall-list.h'
syscall_list_h = custom_target(
        fname,
        input : syscall_list_txt,
        output : fname,
        command : [generate_syscall_list,
                   '@INPUT@'],
        capture : true)

if conf.get('HAVE_ACL') == 1
        shared_sources += files('''
                acl-util.c
                devnode-acl.c
        '''.split())
endif

if conf.get('ENABLE_UTMP') == 1
        shared_sources += files('utmp-wtmp.c')
endif

#if 0 /// UNNEEDED by elogind
# if conf.get('HAVE_SECCOMP') == 1
#         shared_sources += files('seccomp-util.c')
#         shared_sources += syscall_list_h
# endif
#
# if conf.get('HAVE_LIBIPTC') == 1
#         shared_sources += files('firewall-util-iptables.c')
# endif
#endif // 0

#if 0 /// elogind does not play around with kernel modules. Not its job.
# if conf.get('HAVE_KMOD') == 1
#         shared_sources += files('module-util.c')
# endif
#endif // 0

if conf.get('HAVE_PAM') == 1
        shared_sources += files(
                'pam-util.c',
                'pam-util.h')
endif

#if 0 /// elogind does not need all this networking stuff
# if conf.get('ENABLE_NSCD') == 1
#         shared_sources += files('nscd-flush.c')
# endif
#
# generate_ip_protocol_list = find_program('generate-ip-protocol-list.sh')
# ip_protocol_list_txt = custom_target(
#         'ip-protocol-list.txt',
#         output : 'ip-protocol-list.txt',
#         command : [generate_ip_protocol_list, cpp],
#         capture : true)
#
# fname = 'ip-protocol-from-name.gperf'
# gperf_file = custom_target(
#         fname,
#         input : ip_protocol_list_txt,
#         output : fname,
#         command : [generate_gperfs, 'ip_protocol', 'IPPROTO_', '@INPUT@'],
#         capture : true)
#
# fname = 'ip-protocol-from-name.h'
# target1 = custom_target(
#         fname,
#         input : gperf_file,
#         output : fname,
#         command : [gperf,
#                    '-L', 'ANSI-C', '-t', '--ignore-case',
#                    '-N', 'lookup_ip_protocol',
#                    '-H', 'hash_ip_protocol_name',
#                    '-p', '-C',
#                    '@INPUT@'],
#         capture : true)
#
# fname = 'ip-protocol-to-name.h'
# awkscript = 'ip-protocol-to-name.awk'
# target2 = custom_target(
#         fname,
#         input : [awkscript, ip_protocol_list_txt],
#         output : fname,
#         command : [awk, '-f', '@INPUT0@', '@INPUT1@'],
#         capture : true)
#
# shared_generated_gperf_headers = [target1, target2]
# shared_sources += shared_generated_gperf_headers
#endif // 0

libshared_name = 'elogind-shared-@0@'.format(meson.project_version())

#if 0 /// elogind doesn't need all this
# libshared_deps = [threads,
#                   libacl,
#                   libblkid,
#                   libcap,
#                   libcrypt,
#                   libgcrypt,
#                   libiptc,
#                   libkmod,
#                   liblz4,
#                   libmount,
#                   libopenssl,
#                   libp11kit,
#                   libpam,
#                   librt,
#                   libseccomp,
#                   libselinux,
#                   libzstd,
#                   libxz]
#else // 0
libshared_deps = [threads,
                  libacl,
                  libcap,
                  libmount,
                  libpam,
                  libselinux]
#endif // 0

libshared_sym_path = '@0@/libshared.sym'.format(meson.current_source_dir())

libshared_static = static_library(
        libshared_name,
        shared_sources,
        include_directories : includes,
        dependencies : libshared_deps,
        c_args : ['-fvisibility=default'])

libshared = shared_library(
        libshared_name,
        include_directories : includes,
        link_args : ['-shared',
                     '-Wl,--version-script=' + libshared_sym_path],
        link_whole : [libshared_static,
                      libbasic,
#if 0 /// UNNEEDED by elogind
#                       libbasic_gcrypt,
#                       libsystemd_static],
#else // 0
                      libelogind_static],
#endif // 0
        c_args : ['-fvisibility=default'],
        dependencies : libshared_deps,
        install : true,
        install_dir : rootlibexecdir)
