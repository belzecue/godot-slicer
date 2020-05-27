#include "slicer.h"
#include "utils/slicer_face.h"
#include "utils/intersector.h"
#include "utils/triangulator.h"

Ref<SlicedMesh> Slicer::slice(const Ref<Mesh> mesh, const Plane plane, const Ref<Material> cross_section_material) {
    if (mesh.is_null()) {
        return Ref<SlicedMesh>();
    }

    PoolVector<Intersector::SplitResult> split_results;
    split_results.resize(mesh->get_surface_count());
    PoolVector<Intersector::SplitResult>::Write split_results_writer = split_results.write();

    // Intersection points are shared across all surfaces
    PoolVector<Vector3> intersection_points;

    for (int i = 0; i < mesh->get_surface_count(); i++) {
        Intersector::SplitResult results = split_results[i];

        results.material = mesh->surface_get_material(i);
        PoolVector<SlicerFace> faces = SlicerFace::faces_from_surface(**mesh, i);
        PoolVector<SlicerFace>::Read faces_reader = faces.read();

        for (int j = 0; j < faces.size(); j++) {
            Intersector::split_face_by_plane(plane, faces_reader[j], results);
        }

        int ip_size = intersection_points.size();
        intersection_points.resize(ip_size + results.intersection_points.size());
        PoolVector<Vector3>::Write ip_writer = intersection_points.write();
        for (int j = 0; j < results.intersection_points.size(); j ++) {
            ip_writer[ip_size + j] = results.intersection_points[j];
        }
        ip_writer.release();
        results.intersection_points.resize(0);

        // Save the updated result back into the array (PoolVector doesn't seem to have a non
        // const index operator)
        split_results_writer[i] = results;
    }


    // If no intersection has occurred then just return
    if (intersection_points.size() == 0) {
        return Ref<SlicedMesh>();
    }

    PoolVector<SlicerFace> cross_section_faces = Triangulator::monotone_chain(intersection_points, plane.normal);

    SlicedMesh *sliced_mesh = memnew(SlicedMesh(split_results, cross_section_faces, cross_section_material));
    return Ref<SlicedMesh>(sliced_mesh);
}

void Slicer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("slice", "mesh", "plane", "cross_section_material"), &Slicer::slice, Variant::NIL);
}

Slicer::Slicer() {
}