
#include "Tools.h"

#include <iostream>

void error_handler(void* userPtr, const RTCError code, const char* str)
{
	if (code == RTC_ERROR_NONE)
		return;

	printf("Embree: ");
	switch (code)
	{
	case RTC_ERROR_UNKNOWN:
		printf("RTC_ERROR_UNKNOWN");
		break;
	case RTC_ERROR_INVALID_ARGUMENT:
		printf("RTC_ERROR_INVALID_ARGUMENT");
		break;
	case RTC_ERROR_INVALID_OPERATION:
		printf("RTC_ERROR_INVALID_OPERATION");
		break;
	case RTC_ERROR_OUT_OF_MEMORY:
		printf("RTC_ERROR_OUT_OF_MEMORY");
		break;
	case RTC_ERROR_UNSUPPORTED_CPU:
		printf("RTC_ERROR_UNSUPPORTED_CPU");
		break;
	case RTC_ERROR_CANCELLED:
		printf("RTC_ERROR_CANCELLED");
		break;
	default:
		printf("invalid error code");
		break;
	}
	if (str)
	{
		printf(" (");
		while (*str)
			putchar(*str++);
		printf(")\n");
	}
	exit(1);
}

/* prints the bvh4.triangle4v data structure */
void print_bvh4_triangle4v(embree::BVH4::NodeRef node, size_t depth)
{
	if (node.isAABBNode())
	{
		embree::BVH4::AABBNode* n = node.getAABBNode();

		std::cout << "AABBNode {" << std::endl;
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t k = 0; k < depth; k++)
				std::cout << "  ";
			std::cout << "  bounds" << i << " = " << n->bounds(i) << std::endl;
		}

		for (size_t i = 0; i < 4; i++)
		{
			if (n->child(i) == embree::BVH4::emptyNode)
				continue;

			for (size_t k = 0; k < depth; k++)
				std::cout << "  ";
			std::cout << "  child" << i << " = ";
			print_bvh4_triangle4v(n->child(i), depth + 1);
		}
		for (size_t k = 0; k < depth; k++)
			std::cout << "  ";
		std::cout << "}" << std::endl;
	}
	else
	{
		size_t			  num;
		const embree::Triangle4v* tri = (const embree::Triangle4v*)node.leaf(num);

		std::cout << "Leaf {" << std::endl;
		for (size_t i = 0; i < num; i++)
		{
			for (size_t j = 0; j < tri[i].size(); j++)
			{
				for (size_t k = 0; k < depth; k++)
					std::cout << "  ";
				std::cout << "  Triangle { v0 = (" << tri[i].v0.x[j] << ", " << tri[i].v0.y[j] << ", " << tri[i].v0.z[j] << "),  "
																															"v1 = ("
						  << tri[i].v1.x[j] << ", " << tri[i].v1.y[j] << ", " << tri[i].v1.z[j] << "), "
																								   "v2 = ("
						  << tri[i].v2.x[j] << ", " << tri[i].v2.y[j] << ", " << tri[i].v2.z[j] << "), "
																								   "geomID = "
						  << tri[i].geomID(j) << ", primID = " << tri[i].primID(j) << " }" << std::endl;
			}
		}
		for (size_t k = 0; k < depth; k++)
			std::cout << "  ";
		std::cout << "}" << std::endl;
	}
}

/* prints the triangle BVH of a scene */
void print_bvh(RTCScene scene)
{
	embree::BVH4* bvh4 = nullptr;

	/* if the scene contains only triangles, the BVH4 acceleration structure can be obtained this way */
	embree::AccelData* accel = ((embree::Accel*)scene)->intersectors.ptr;
	if (accel->type == embree::AccelData::TY_BVH4)
		bvh4 = (embree::BVH4*)accel;

	/* if there are also other geometry types, one has to iterate over the toplevel AccelN structure */
	else if (accel->type == embree::AccelData::TY_ACCELN)
	{
		embree::AccelN* accelN = (embree::AccelN*)(accel);
		for (size_t i = 0; i < accelN->accels.size(); i++)
		{
			if (accelN->accels[i]->intersectors.ptr->type == embree::AccelData::TY_BVH4)
			{
				bvh4 = (embree::BVH4*)accelN->accels[i]->intersectors.ptr;
				if (std::string(bvh4->primTy->name()) == "triangle4v")
					break;
				bvh4 = nullptr;
			}
		}
	}
	if (bvh4 == nullptr)
		throw std::runtime_error("cannot access BVH4 acceleration structure"); // will not happen if you use this Embree version

	/* now lets print the entire hierarchy */
	print_bvh4_triangle4v(bvh4->root, 0);
}