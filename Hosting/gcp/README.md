# Google Cloud GPU Deployment

This deployment targets one browser player on one G2 VM with an NVIDIA L4 RTX
Virtual Workstation GPU. It uses Ubuntu 22.04, a private custom VPC, OS Login,
IAP-only SSH, a static public address, and only the public Pixel Streaming ports.

No project ID, account, token, TLS key, TURN secret, or packaged game is committed.
Every action that can create resources, begin charges, deploy code, or delete
resources requires an explicit `--apply` argument. Stopping the VM is immediate
so the cost-control path cannot be blocked by a confirmation flag.

## Current Cost Boundary

The default `g2-standard-8` has 8 vCPUs, 32 GiB RAM, and one L4. Google lists
its on-demand VM price at about `$0.8536/hour`; the L4 RTX Virtual Workstation
license is listed at about `$0.5600/hour`. The combined running baseline is
therefore about `$1.4137/hour` before the 150 GB disk, static IPv4 address, and
internet egress. Streaming video egress can be a material additional cost.

Prices vary by region and can change. Recheck Google's pricing pages immediately
before creation. Stopping the VM stops VM/GPU/vWS runtime charges, but its disk
and reserved IP continue billing. `destroy.sh` removes all resources created by
these scripts.

- [G2 VM pricing](https://cloud.google.com/products/compute/pricing/accelerator-optimized)
- [RTX Virtual Workstation pricing](https://cloud.google.com/products/compute/gpus-pricing)
- [GPU quota requirements](https://docs.cloud.google.com/compute/resource-usage#gpu_quota)

## Prerequisites

1. Install and authenticate the Google Cloud CLI.
2. Enable billing and the required Google Cloud services in the target project.
3. Request at least one `GPUs (all regions)` quota.
4. Request at least one `NVIDIA L4 virtual workstation GPUs` quota in the region.
5. Package the project for Linux using Unreal Engine 5.8 on a Linux build machine.
6. Control a DNS name that can point to the reserved address.

Google's quota page is:

`https://console.cloud.google.com/iam-admin/quotas`

## Configure And Check

```bash
cp Hosting/gcp/gcp.env.example Hosting/gcp/gcp.env
# Set the project ID and deployment values in gcp.env.
./Hosting/gcp/bin/enable_services.sh --apply
./Hosting/gcp/bin/preflight.sh
```

The recommended Phoenix-area targets are `us-west1` (Oregon) and `us-west4`
(Las Vegas), subject to quota and live capacity. The default is `us-west1-b`.

## Create The Paid Host

Review the scripts and current price first. Then:

```bash
./Hosting/gcp/bin/create_foundation.sh --apply
./Hosting/gcp/bin/create_gpu_vm.sh --apply
./Hosting/gcp/bin/instance.sh logs
```

Driver installation can reboot the VM twice. Wait until the bootstrap log reports
that host prerequisites are ready. Point the stream domain's DNS A record at the
static address printed by `create_foundation.sh`.

## Upload And Publish

Package the Linux build as a `.tar.gz`, then run:

```bash
./Hosting/gcp/bin/upload_package.sh --apply /path/to/GorillaProtocol-Linux.tar.gz
./Hosting/gcp/bin/deploy_host.sh --apply
```

The hosting stack can come online without a game and display a waiting player,
but gameplay will not stream until `GAME_BINARY` points to a valid executable.

## Cost Controls

```bash
./Hosting/gcp/bin/instance.sh status
./Hosting/gcp/bin/instance.sh stop
./Hosting/gcp/bin/instance.sh start --apply
./Hosting/gcp/bin/destroy.sh --apply your-gcp-project-id
```

Do not use Spot for the public demo: an interruption immediately ends the player's
session. Standard provisioning is intentional for this single-instance baseline.
