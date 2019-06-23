#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

typedef struct ObscuraMaterial {
	enum {
		OBSCURA_MATERIAL_TYPE_COLOR,
		OBSCURA_MATERIAL_TYPE_TEXTURE,
	}	 type;
	void	*material;
} ObscuraMaterial;

#endif
