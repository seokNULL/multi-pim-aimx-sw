#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x602c1205, "module_layout" },
	{ 0xe0a54b97, "cdev_alloc" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xa2cc0def, "kmem_cache_destroy" },
	{ 0x65561d2e, "cdev_del" },
	{ 0x41964dfe, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xbbcadeba, "cdev_init" },
	{ 0x5cc64681, "put_devmap_managed_page" },
	{ 0x3e4e346c, "genl_register_family" },
	{ 0xcfd22645, "debugfs_create_dir" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x91eb9b4, "round_jiffies" },
	{ 0x754d539c, "strlen" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0xddbb6612, "genl_unregister_family" },
	{ 0x48d490a8, "dma_set_mask" },
	{ 0x8b1c1f21, "pcie_set_readrq" },
	{ 0x1aab7c0b, "pci_disable_device" },
	{ 0xdc366470, "pci_disable_msix" },
	{ 0xc3690fc, "_raw_spin_lock_bh" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0xc70cf906, "device_destroy" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xdc409315, "pci_release_regions" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xc037857d, "pcie_capability_clear_and_set_word" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x85df9b6c, "strsep" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x167e7f9d, "__get_user_1" },
	{ 0xe8ccf275, "pcie_get_readrq" },
	{ 0x97892f52, "dma_free_attrs" },
	{ 0x716589cc, "debugfs_create_file" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x5abd1a39, "sysfs_remove_group" },
	{ 0x84f243a6, "pv_ops" },
	{ 0x9de926, "set_page_dirty" },
	{ 0xcce0d0b6, "dma_set_coherent_mask" },
	{ 0x683eadc2, "kthread_create_on_node" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x7d0700e1, "param_ops_string" },
	{ 0x9fbc2080, "kthread_bind" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb28c58df, "pci_set_master" },
	{ 0xf13bca23, "__alloc_pages" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0xfb578fc5, "memset" },
	{ 0x91889b2f, "pci_restore_state" },
	{ 0x82fe7e83, "pci_iounmap" },
	{ 0x8518a4a6, "_raw_spin_trylock_bh" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x2aee72c, "kthread_stop" },
	{ 0x607f5984, "sysfs_create_group" },
	{ 0xe0ebd81a, "pci_aer_clear_nonfatal_status" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0x531b604e, "__virt_addr_valid" },
	{ 0x9166fada, "strncpy" },
	{ 0xfacd8c28, "nla_put" },
	{ 0x323c3952, "debugfs_remove" },
	{ 0x3cef829f, "dma_alloc_attrs" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x9ebb3451, "kfree_skb_reason" },
	{ 0x6c3d6e4, "device_create" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x61b37490, "netlink_unicast" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0x3e3d3caa, "_dev_err" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x2f853d55, "cdev_add" },
	{ 0x800473f, "__cond_resched" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x167c5967, "print_hex_dump" },
	{ 0x50f4a523, "_dev_info" },
	{ 0x17d755db, "__free_pages" },
	{ 0x6383b27c, "__x86_indirect_thunk_rdx" },
	{ 0x618911fc, "numa_node" },
	{ 0xb248d598, "__alloc_skb" },
	{ 0xa916b694, "strnlen" },
	{ 0xf17a9143, "pci_enable_msix_range" },
	{ 0xfe916dc6, "hex_dump_to_buffer" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x1000e51, "schedule" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x92997ed8, "_printk" },
	{ 0x643ead61, "dma_map_page_attrs" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x284faa6b, "__x86_indirect_thunk_r11" },
	{ 0x481ead8c, "wake_up_process" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x235ed0d8, "pci_unregister_driver" },
	{ 0x5bb7b46f, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xb19a5453, "__per_cpu_offset" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x9a66c85b, "kmem_cache_create" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xdc0e4855, "timer_delete" },
	{ 0x37a0cba, "kfree" },
	{ 0x3b6c41ea, "kstrtouint" },
	{ 0x69acdf38, "memcpy" },
	{ 0x44dbcd40, "genlmsg_put" },
	{ 0xf7d759d8, "pci_request_regions" },
	{ 0x6df1aaf1, "kernel_sigaction" },
	{ 0x42b10c45, "set_user_nice" },
	{ 0x3eff7cb3, "__pci_register_driver" },
	{ 0x96848186, "scnprintf" },
	{ 0x480559c7, "class_destroy" },
	{ 0xa1496fdf, "dma_unmap_page_attrs" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x1ba59527, "__kmalloc_node" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xb6a9bd82, "pci_iomap" },
	{ 0xd6e9698a, "vmalloc_to_page" },
	{ 0xa6522fc4, "pci_enable_device_mem" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0x9cdd4d33, "pci_enable_device" },
	{ 0x9b315ff2, "pci_msix_vec_count" },
	{ 0xadb4ed75, "param_ops_uint" },
	{ 0x755ddd79, "__class_create" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xc5cca712, "__put_page" },
	{ 0x84e6b5fa, "get_user_pages_fast" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x4086aac2, "pci_save_state" },
	{ 0x9c6febfc, "add_uevent_var" },
	{ 0x587f22d7, "devmap_managed_key" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v000010EEd0000A011sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A111sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A211sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A311sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A012sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A112sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A212sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A312sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A014sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A114sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A214sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A314sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A018sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A118sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A218sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A318sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A01Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A11Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A21Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A31Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A021sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A121sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A221sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A321sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A022sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A122sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A222sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A322sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A024sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A124sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A224sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A324sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A028sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A128sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A228sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A328sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A02Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A12Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A22Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A32Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A031sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A131sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A231sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A331sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A032sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A132sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A232sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A332sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A034sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A134sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A234sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A334sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A038sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A138sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A238sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A338sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A03Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A13Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A23Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A33Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A041sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A141sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A241sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A341sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A042sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A142sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A242sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A342sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A044sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A144sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A244sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A344sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A444sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A544sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A644sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A744sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A048sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A148sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A248sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000A348sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C011sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C111sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C211sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C311sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C012sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C112sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C212sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C312sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C014sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C114sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C214sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C314sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C018sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C118sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C218sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C318sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C01Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C11Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C21Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C31Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C021sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C121sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C221sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C321sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C022sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C122sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C222sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C322sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C024sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C124sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C224sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C324sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C028sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C128sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C228sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C328sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C02Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C12Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C22Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C32Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C031sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C131sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C231sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C331sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C032sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C132sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C232sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C332sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C034sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C134sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C234sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C334sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C038sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C138sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C238sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C338sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C03Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C13Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C23Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C33Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C041sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C141sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C241sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C341sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C042sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C142sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C242sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C342sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C044sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C144sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C244sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C344sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C444sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C544sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C644sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C744sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C048sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C148sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C248sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000C348sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "6DEA65082A4C682F218AA10");
